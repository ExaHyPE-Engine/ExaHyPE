// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEvent.h"

#include "tarch/services/ServiceRepository.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static_assert(std::is_standard_layout_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridControlEvent>);

namespace {
  peano4::grid::GridControlEvent createBoundingBoxEvent(
    const peano4::grid::GridControlEvent& lhs,
    const peano4::grid::GridControlEvent& rhs
  ) {
    auto result   = rhs;
    result.offset = tarch::la::min(lhs.offset, rhs.offset);
    result.width  = tarch::la::max(lhs.offset + lhs.width, rhs.offset + rhs.width) - result.offset;
    return result;
  };

  bool twoEventsOverlap(const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs) {
    for (int d = 0; d < DIMENSIONS; ++d) {
      if (lhs.offset[d] + lhs.width[d] < rhs.offset[d]) {
        return false;
      }
      if (rhs.offset[d] + rhs.width[d] < lhs.offset[d]) {
        return false;
      }
    }
    return true;
  }

  bool refinementEventOverrulesCoarsening(
    const peano4::grid::GridControlEvent& refineEvent,
    const peano4::grid::GridControlEvent& eraseEvent
  ) {
    if (refineEvent.getRefinementControl() != peano4::grid::GridControlEvent::RefinementControl::Refine) {
      return false;
    }
    if (eraseEvent.getRefinementControl() != peano4::grid::GridControlEvent::RefinementControl::Erase) {
      return false;
    }
    if (not twoEventsOverlap(refineEvent, eraseEvent)) {
      return false;
    }
    return tarch::la::oneSmallerEquals((1.0 / 3.0) * refineEvent.h, eraseEvent.h);
  };

  void removeEraseEventsThatAreCancelledByRefineEvents(
    const std::list<peano4::grid::GridControlEvent>& refineEvents,
    std::list<peano4::grid::GridControlEvent>&       eraseEvents
  ) {
    std::list<peano4::grid::GridControlEvent>::iterator pEraseEvent = eraseEvents.begin();
    while (pEraseEvent != eraseEvents.end()) {
      bool overlaps = false;
      for (const auto& refineEvent : refineEvents) {
        overlaps |= refinementEventOverrulesCoarsening(refineEvent, *pEraseEvent);
      }
      if (overlaps) {
        pEraseEvent = eraseEvents.erase(pEraseEvent);
      } else {
        pEraseEvent++;
      }
    }
  }

  bool twoEventsAreAdjacent(
    const peano4::grid::GridControlEvent& lhs,
    const peano4::grid::GridControlEvent& rhs,
    double                                tolerance
  ) {
    const double relTol = tarch::la::relativeEps(tarch::la::volume(lhs.width), tarch::la::volume(rhs.width), tolerance);
    if (not tarch::la::equals(lhs.h, rhs.h, relTol)) {
      return false;
    }

    const auto lo     = tarch::la::min(lhs.offset, rhs.offset);
    const auto hi     = tarch::la::max(lhs.offset + lhs.width, rhs.offset + rhs.width);
    const auto unionW = hi - lo;

    // volume(union) <= vol(a)+vol(b) within tolerance  => face-touch or overlap
    return tarch::la::
      smallerEquals(tarch::la::volume(unionW), tarch::la::volume(lhs.width) + tarch::la::volume(rhs.width), relTol);
  };

  /**
   * Merge adjacent events
   *
   * This routine merges adjacent events, i.e. events of the same time which
   * share a common face and can be combined into one larger, rectangular
   * event. The routine does not distinguish refine from coarsen events, i.e.
   * you should only pass in events of the same type.
   *
   * To make this routine faster, we first sort() the events geometrically.
   * After that, we sweep over them and merge. As we have sorted the elements,
   * we can "only" consider neighbouring events. Once a sweep has completed,
   * we have to decide to rerun the whole procedure. This is necessary if
   * something has changed within the list.
   *
   */
  void mergeAdjacentRefinementEvents(std::list<peano4::grid::GridControlEvent>& events, double tolerance) {
    if (events.empty()) {
      return;
    }

    bool hasMergedEvents = true;
    while (hasMergedEvents) {
      events.sort();
      hasMergedEvents = false;

      std::list<peano4::grid::GridControlEvent>::iterator lhs = events.begin();
      std::list<peano4::grid::GridControlEvent>::iterator rhs = events.begin();
      rhs++;

      while (rhs != events.end()) {
        if (twoEventsAreAdjacent(*rhs, *lhs, tolerance)) {
          peano4::grid::GridControlEvent newEvent = createBoundingBoxEvent(*rhs, *lhs);
          *lhs                                    = newEvent;
          lhs                                     = events.erase(rhs);
          rhs                                     = lhs;
          if (rhs != events.end()) {
            rhs++;
          }
          hasMergedEvents = true;
        } else {
          lhs++;
          rhs++;
        }
      }
    }
  }
} // namespace

bool peano4::grid::GridControlEvent::operator<(const GridControlEvent& other) const noexcept {
  if (tarch::la::smaller(tarch::la::volume(this->h), tarch::la::volume(other.h))) {
    return true;
  }
  for (int d = 0; d < DIMENSIONS; d++) {
    if (tarch::la::smaller(this->offset[d], other.offset[d])) {
      return true;
    }
    if (tarch::la::greater(this->offset[d], other.offset[d])) {
      return false;
    }
  }
  return false;
}

/**
 * Merge set of refinement/coarsening commands
 *
 * This routine has two roles:
 * - It shall reduce the number of refine events, so the actual AMR
 *   checks are faster when we next trigger the traversal
 * - It brings together the erase events: Erases are only triggered
 *   if a cell is completely embedded into an event. See
 *   Spacetree::evaluateGridControlEvents() for details. However, if
 *   we have two adjacent erase events, then it might happen that none
 *   of them hosts a cell, while the merger of the two of them does
 *   host one.
 *
 *
 * ## Algorithm
 *
 * The algorithm works in multiple steps:
 *
 * - We separate erase from refine events.
 * - We remove those erase events which are overwritten by refinement
 *   events. As we work conservatively, refinement events are always
 *   more important than coarsening ones.
 * - Sort the events
 * - Create the erase power set.
 * - The code is asked to fuse multiple adjacent refinement events into
 *   fewer large ones, and to fuse multiple coarsening events as well.
 *   This reduces the event count, and gives the coarsening more
 *   flexibility (see discussion above).
 * - We join the remaining erase and refinement lists.
 *
 *
 * ## Sorting
 *
 * We sort the respective events along x,y,z offset. In 2d, this doesn't
 * make that much of a difference, for d>2 it is important: The merger of
 * multiple events works in a greedy fashion. Otherwise, it would be too
 * slow. Due to this greediness, we now might run into situations where we
 * are stuck. Consider a 2x2x2 cube of refinement operations which all have
 * the same target resolution. It should be possible to merge this cube
 * into one single large cube. However, we might end up with
 *
 *        (r_111,r_211), (r_121,r_122), (r_212,r_222), (r_112, r221)
 *
 * These six events now can't be merged greedily anymore.
 *
 * Once we sort the events by x,y,z, the original input sequence implicitly
 * ensures that we first merge along the z direction, then along y, and
 * finally along x:
 *
 *        (r_111,r1_112,r_121,r_122,r_211,r1_212,r_221,r_222)
 *
 *
 * ##  Erasing the mesh (again)
 *
 * We have to create the power sets over all
 * erase events. This gives the mesh the opportunity to see faster
 * where erases are possible, as it also handle non-rectangular
 * regions: Assume there's a large L-shape of erase instructions consisting
 * of three erase commands. Our algorithm will create a rectangle
 * plus a square when it fuses the erases.
 *
 * Consequently, any cell overlapping with the boundary between the square
 * and the rectangle will not be erased.
 *
 * If we had not merged the three cubic erases forming the L into 2+1 but
 * into 2+2, we would also cover the boundary of the L shape. Therefore,
 * it is important that we construct the power set before we merge events.
 *
 * The power set construction very quickly explodes, i.e. leads to a vast
 * number of events. I thus manually truncate the creation of new events.
 *
 * Remark: I had to remove this power set step, as it turned out to be too
 * time consuming.
 *
 *
 * ## Properties
 *
 * The algorithm has to be idempotent, i.e. if we invoke it multiple times
 * over the input set, the outcome should always remain the same.
 *
 * @param events Set of events which we try to merge
 *
 * @param tolerance Relative tolerance passed into the actual merger, i.e.
 *   the tolerance at which two events are merged, even though they might
 *   not exactly be adjacent. A default of 10% (relative tolerance) is
 *   sufficient for most cases, but there might be situation where a larger
 *   tolerance is reasonable to ensure that some events which are slightly
 *   disjoint are actually merged, too.
 */
void peano4::grid::GridControlEvent::merge(std::vector<GridControlEvent>& events, double tolerance) {
  if (events.empty()) {
    return;
  }

  std::list<peano4::grid::GridControlEvent> refineEvents;
  std::list<peano4::grid::GridControlEvent> eraseEvents;

  for (const auto& p : events) {
    if (p.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      eraseEvents.push_back(p);
    } else {
      refineEvents.push_back(p);
    }
  }

  removeEraseEventsThatAreCancelledByRefineEvents(refineEvents, eraseEvents);

  mergeAdjacentRefinementEvents(refineEvents, tolerance);
  mergeAdjacentRefinementEvents(eraseEvents, tolerance);

  events.clear();
  events.reserve(refineEvents.size() + eraseEvents.size());
  events
    .insert(events.end(), std::make_move_iterator(refineEvents.begin()), std::make_move_iterator(refineEvents.end()));
  events.insert(events.end(), std::make_move_iterator(eraseEvents.begin()), std::make_move_iterator(eraseEvents.end()));
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridControlEvent& event) {
  return (out << event.toString());
}

std::string peano4::grid::GridControlEvent::toString() const {
  auto rc2str = [](RefinementControl rc) { return (rc == RefinementControl::Refine) ? "Refine" : "Erase"; };

  std::ostringstream out;
  out << "GridControlEvent{";
  out << "RefinementControl=" << rc2str(getRefinementControl());

  out << ", Offset=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << offset[d];
  }
  out << "]";

  out << ", Width=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << width[d];
  }
  out << "]";

  out << ", h=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << h[d];
  }
  out << "]}";
  return out.str();
}

#ifdef WITH_MPI
MPI_Datatype peano4::grid::GridControlEvent::Datatype = MPI_DATATYPE_NULL;

MPI_Datatype peano4::grid::GridControlEvent::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridControlEvent::getGlobalCommunicationDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridControlEvent::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridControlEvent::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridControlEvent::getMultiscaleDataExchangeDatatype() { return Datatype; }

void peano4::grid::GridControlEvent::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridControlEvent::freeGlobalCommunicationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridControlEvent::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridControlEvent::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridControlEvent::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridControlEvent::initDatatype() {
  GridControlEvent instances[2];

  constexpr int NumberOfAttributes = 4;
  MPI_Datatype  subtypes[NumberOfAttributes];
  int           blocklen[NumberOfAttributes];
  MPI_Aint      disp[NumberOfAttributes];

  int counter = 0;

  subtypes[counter] = MPI_UINT8_T;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].refinementControl, &disp[counter]);
  ++counter;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].offset.data()[0]), &disp[counter]);
  ++counter;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].width.data()[0]), &disp[counter]);
  ++counter;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].h.data()[0]), &disp[counter]);
  ++counter;

  MPI_Aint baseFirstInstance;
  MPI_Aint baseSecondInstance;
  MPI_Get_address(&instances[0], &baseFirstInstance);
  MPI_Get_address(&instances[1], &baseSecondInstance);
  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i = NumberOfAttributes - 1; i >= 0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int          errorCode = 0;
  MPI_Datatype tmpType;
  errorCode += MPI_Type_create_struct(NumberOfAttributes, blocklen, disp, subtypes, &tmpType);
  errorCode += MPI_Type_create_resized(tmpType, offset, extent, &Datatype);
  errorCode += MPI_Type_commit(&Datatype);
  errorCode += MPI_Type_free(&tmpType);

  if (errorCode) {
    std::stringstream ss;
    ss << "Error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
    throw std::runtime_error(ss.str());
  }
}

void peano4::grid::GridControlEvent::shutdownDatatype() {
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
}

void peano4::grid::GridControlEvent::
  send(const GridControlEvent& buffer, int destination, int tag, MPI_Comm communicator) {
  MPI_Send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridControlEvent::receive(GridControlEvent& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  MPI_Recv(&buffer, 1, Datatype, source, tag, communicator, &status);
  buffer.senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridControlEvent::send(
  const GridControlEvent& buffer,
  int                     destination,
  int                     tag,
  std::function<void()>   startCommunicationFunctor,
  std::function<void()>   waitFunctor,
  MPI_Comm                communicator
) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend(&buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle);
  MPI_Test(&sendRequestHandle, &flag, MPI_STATUS_IGNORE);
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test(&sendRequestHandle, &flag, MPI_STATUS_IGNORE);
  }
}

void peano4::grid::GridControlEvent::receive(
  GridControlEvent&     buffer,
  int                   source,
  int                   tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm              communicator
) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv(&buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle);
  MPI_Test(&receiveRequestHandle, &flag, &status);
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test(&receiveRequestHandle, &flag, &status);
  }
  buffer.senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridControlEvent::
  sendAndPollDanglingMessages(const GridControlEvent& message, int destination, int tag, MPI_Comm communicator) {
  GridControlEvent::send(
    message,
    destination,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()", destination, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()", destination, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

void peano4::grid::GridControlEvent::
  receiveAndPollDanglingMessages(GridControlEvent& message, int source, int tag, MPI_Comm communicator) {
  GridControlEvent::receive(
    message,
    source,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
