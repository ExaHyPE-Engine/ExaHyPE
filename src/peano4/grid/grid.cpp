#include "grid.h"

#include "GridVertex.h"
#include "AutomatonState.h"
#include "GridControlEvent.h"
#include "GridStatistics.h"
#include "Spacetree.h"
#include "peano4/utils/Loop.h"

namespace {
  [[maybe_unused]] tarch::logging::Log _log("peano4::grid");
} // namespace



std::string peano4::SplitInstruction::toString(peano4::SplitInstruction::Mode mode) {
  switch (mode) {
  case SplitInstruction::Mode::AggressiveTopDown:
    return "aggressive-top-down";
    break;
  case SplitInstruction::Mode::BottomUp:
    return "bottom-up";
    break;
  }
  return "<undef>";
}

std::string peano4::SplitInstruction::toString() const {
  std::ostringstream msg;
  msg << "(" << toString(mode) << ",#cells=" << numberOfFineGridCells << ")";
  return msg.str();
}

std::ostream& operator<<(std::ostream& out, const peano4::SplitInstruction& instruction) {
  out << instruction.toString();
  return out;
}

std::ostream& operator<<(std::ostream& out, const peano4::SplitInstruction::Mode& mode) {
  out << peano4::SplitInstruction::toString(mode);
  return out;
}

bool peano4::grid::isContained(
  const peano4::grid::AutomatonState& state, const peano4::grid::GridControlEvent& event, double upscaleAutomatonState
) {
  assertion1(upscaleAutomatonState >= 1.0, upscaleAutomatonState);
  return tarch::la::
           allGreaterEquals(state.getX() + (0.5 - 0.5 * upscaleAutomatonState) * state.getH(), event.getOffset())
         and tarch::la::allSmallerEquals(
           state.getX() + (0.5 + 0.5 * upscaleAutomatonState) * state.getH(), event.getOffset() + event.getWidth()
         );
}

bool peano4::grid::overlaps(const peano4::grid::AutomatonState& state, const peano4::grid::GridControlEvent& event) {
  return tarch::la::allGreaterEquals(state.getX() + state.getH(), event.getOffset())
         and tarch::la::allSmallerEquals(state.getX(), event.getOffset() + event.getWidth());
}

bool peano4::grid::overlaps(const tarch::la::Vector<Dimensions, double>& x, const GridControlEvent& event) {
  return tarch::la::allGreaterEquals(x, event.getOffset())
         and tarch::la::allSmallerEquals(x, event.getOffset() + event.getWidth());
}

peano4::grid::GridVertex peano4::grid::createVertex(
  [[maybe_unused]] GridVertex::State                            state,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] int                                          level,
  [[maybe_unused]] const tarch::la::Vector<TwoPowerD,int>&      adjacentRanks,
  [[maybe_unused]] bool                                         isNewFineGridVertex
) {
  GridVertex result;

  result.setState(state);
  result.setAdjacentRanks(adjacentRanks);
  result.setBackupOfAdjacentRanks(adjacentRanks);
  result.setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(not isNewFineGridVertex);
  result.setIsAntecessorOfRefinedVertexInCurrentTreeSweep(not isNewFineGridVertex);
  result.setHasBeenParentOfSubtreeVertexInPreviousTreeSweep(false);
  result.setIsParentOfSubtreeVertexInCurrentTreeSweep(false);

#if PeanoDebug > 0
  result.setX(x);
#endif
  result.setLevel(level);

  // not required logically, but makes valgrind's memchecker happy
  result.setNumberOfAdjacentRefinedLocalCells(0);

  return result;
}

std::string peano4::grid::toString(const std::vector<GridControlEvent>& events) {
  std::ostringstream msg;
  msg << "(";
  for (int i = 0; i < static_cast<int>(events.size()); i++) {
    if (i > 0) {
      msg << ",";
    }
    msg << events[i].toString();
  }
  msg << ")";
  return msg.str();
}

std::string peano4::grid::toString(const std::list<GridControlEvent>& events) {
  std::ostringstream msg;
  msg << "[";
  for (auto p : events) {
    msg << p.toString();
  }
  msg << "]";
  return msg.str();
}

std::vector<peano4::grid::GridControlEvent> peano4::grid::merge(
  std::vector<GridControlEvent> events, [[maybe_unused]] const double Tolerance
) {
  logTraceInWith1Argument("merge(...)", events.size());

  std::list<peano4::grid::GridControlEvent> refineEvents;
  std::list<peano4::grid::GridControlEvent> eraseEvents;
  for (auto p : events) {
    if (p.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      eraseEvents.push_back(p);
    } else {
      refineEvents.push_back(p);
    }
  }
  logDebug(
    "merge(...)",
    "have a total of "
      << events.size() << " event(s) consisting of " << refineEvents.size() << " refine and " << eraseEvents.size()
      << " erase events"
  );

  internal::removeEraseEventsThatAreCancelledByRefineEvents(refineEvents, eraseEvents);
  logDebug(
    "merge(...)",
    "reduced to " << eraseEvents.size() << " erase event(s) by eliminating erase events that overlap with refinement"
  );

  internal::mergeAdjacentRefinementEvents(refineEvents, Tolerance);
  internal::mergeAdjacentRefinementEvents(eraseEvents, Tolerance);
  logDebug("merge(...)", "merged refine events into " << refineEvents.size() << " event(s)");

  std::vector<peano4::grid::GridControlEvent> result;
  result.insert(result.begin(), refineEvents.begin(), refineEvents.end());
  result.insert(result.begin(), eraseEvents.begin(), eraseEvents.end());
  logTraceOutWith1Argument("merge(...)", result.size());
  return result;
}

void peano4::grid::internal::sort(std::list<peano4::grid::GridControlEvent>& events) {

  auto compare = [](const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs) -> bool {
    if ( tarch::la::smaller( tarch::la::volume(lhs.getH()), tarch::la::volume(rhs.getH()) ) ) return true; 
    for (int d = 0; d < Dimensions; d++) {
      if (tarch::la::smaller(lhs.getOffset(d), rhs.getOffset(d)))
        return true;
      if (tarch::la::greater(lhs.getOffset(d), rhs.getOffset(d)))
        return false;
    }
    return false;
  };

  events.sort(compare);
}

void peano4::grid::internal::removeEraseEventsThatAreCancelledByRefineEvents(
  const std::list<peano4::grid::GridControlEvent>& refineEvents, std::list<peano4::grid::GridControlEvent>& eraseEvents
) {
  std::list<peano4::grid::GridControlEvent>::iterator pEraseEvent = eraseEvents.begin();
  while (pEraseEvent != eraseEvents.end()) {
    bool overlaps = false;
    for (auto refineEvent : refineEvents) {
      overlaps |= refinementEventOverrulesCoarsening(refineEvent, *pEraseEvent);
    }
    if (overlaps) {
      pEraseEvent = eraseEvents.erase(pEraseEvent);
    } else {
      pEraseEvent++;
    }
  }
}

void peano4::grid::internal::mergeAdjacentRefinementEvents(
  std::list<peano4::grid::GridControlEvent>& inputEvents, int Tolerance
) {
  bool hasMergedEvents = true;
  while (hasMergedEvents) {
    internal::sort(inputEvents);
    hasMergedEvents = false;

    std::list<peano4::grid::GridControlEvent>::iterator lhs = inputEvents.begin();
    std::list<peano4::grid::GridControlEvent>::iterator rhs = inputEvents.begin();
    rhs++;

    while (rhs != inputEvents.end()) {
      if (twoEventsAreAdjacent(*rhs, *lhs, Tolerance)) {
        GridControlEvent newEvent = createBoundingBoxEvent(*rhs, *lhs);
        logDebug(
          "mergeAdjacentRefinementEvents(...)",
          "merge two adjacent events "
            << lhs->toString() << " and " << rhs->toString() << ") into " << newEvent.toString()
        );
        *lhs = newEvent;
        lhs  = inputEvents.erase(rhs);
        rhs  = lhs;
        if (rhs != inputEvents.end())
          rhs++;
        hasMergedEvents = true;
      } else {
        lhs++;
        rhs++;
      }
    }
  }
}

peano4::grid::GridStatistics operator+(peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs) {
  return peano4::grid::GridStatistics(
    lhs.getNumberOfLocalUnrefinedCells() + rhs.getNumberOfLocalUnrefinedCells(),
    lhs.getNumberOfRemoteUnrefinedCells() + rhs.getNumberOfRemoteUnrefinedCells(),
    lhs.getNumberOfLocalRefinedCells() + rhs.getNumberOfLocalRefinedCells(),
    lhs.getNumberOfRemoteRefinedCells() + rhs.getNumberOfRemoteRefinedCells(),
    std::min(lhs.getStationarySweeps(), rhs.getStationarySweeps()),
    lhs.getCoarseningHasBeenVetoed() or rhs.getCoarseningHasBeenVetoed(),
    lhs.getRemovedEmptySubtree() or rhs.getRemovedEmptySubtree(),
    tarch::la::min(lhs.getMinH(), rhs.getMinH())
  );
}

void peano4::grid::clear(GridStatistics& statistics, bool isGlobalMasterTree) {
  statistics.setNumberOfLocalUnrefinedCells(0);
  statistics.setNumberOfRemoteUnrefinedCells(0);
  statistics.setNumberOfLocalRefinedCells(isGlobalMasterTree ? 1 : 0);
  statistics.setNumberOfRemoteRefinedCells(isGlobalMasterTree ? 0 : 1);
  statistics.setCoarseningHasBeenVetoed(false);
  statistics.setRemovedEmptySubtree(false);
  statistics.setStationarySweeps(statistics.getStationarySweeps() + 1);
  statistics.setMinH(tarch::la::Vector<Dimensions, double>(std::numeric_limits<double>::max()));
}

bool peano4::grid::isSpacetreeNodeRefined(GridVertex vertices[TwoPowerD]) {
  bool result = false;
  dfor2(k) result |= willVertexBeRefined(vertices[kScalar]);
  result |= hasVertexBeenRefined(vertices[kScalar]);
  enddforx return result;
}

bool peano4::grid::willVertexBeRefined(const GridVertex& vertex) {
  return vertex.getState() == GridVertex::State::Refining or vertex.getState() == GridVertex::State::Refined
         or vertex.getState() == GridVertex::State::EraseTriggered;
}

bool peano4::grid::hasVertexBeenRefined(const GridVertex& vertex) {
  return vertex.getState() == GridVertex::State::Refined or vertex.getState() == GridVertex::State::EraseTriggered
         or vertex.getState() == GridVertex::State::Erasing;
}

std::bitset<TwoPowerD> peano4::grid::willVerticesBeRefined(GridVertex vertices[TwoPowerD]) {
  std::bitset<TwoPowerD> bitset;
  for (int i = 0; i < TwoPowerD; i++) {
    assertion(not willVertexBeRefined(vertices[i]) or vertices[i].getState() != GridVertex::State::HangingVertex);
    bitset.set(i, willVertexBeRefined(vertices[i]));
  }
  return bitset;
}

std::bitset<TwoPowerD> peano4::grid::haveVerticesBeenRefined(GridVertex vertices[TwoPowerD]) {
  std::bitset<TwoPowerD> bitset;
  for (int i = 0; i < TwoPowerD; i++) {
    assertion(not hasVertexBeenRefined(vertices[i]) or vertices[i].getState() != GridVertex::State::HangingVertex);
    bitset.set(i, hasVertexBeenRefined(vertices[i]));
  }
  return bitset;
}

std::string peano4::grid::toString(VertexType type) {
  switch (type) {
  case VertexType::New:
    return "new";
  case VertexType::Hanging:
    return "hanging";
  case VertexType::Persistent:
    return "persistent";
  case VertexType::Delete:
    return "delete";
  }
  return "<undef>";
}

std::string peano4::grid::toString(FaceType type) {
  switch (type) {
  case FaceType::New:
    return "new";
  case FaceType::Hanging:
    return "hanging";
  case FaceType::Persistent:
    return "persistent";
  case FaceType::Delete:
    return "delete";
  }
  return "<undef>";
}

std::string peano4::grid::toString(CellType type) {
  switch (type) {
  case CellType::New:
    return "new";
  case CellType::Persistent:
    return "persistent";
  case CellType::Delete:
    return "delete";
  }
  return "<undef>";
}

std::string peano4::grid::toString(SpacetreeState state) {
  switch (state) {
  case SpacetreeState::EmptyRun:
    return "empty-run";
  case SpacetreeState::NewRoot:
    return "new-root";
  case SpacetreeState::NewFromSplit:
    return "new-from-split";
  case SpacetreeState::Running:
    return "running";
  case SpacetreeState::JoinTriggered:
    return "join-triggered";
  case SpacetreeState::Joining:
    return "joining";
  case SpacetreeState::Joined:
    return "joined";
  }
  return "<undef>";
}

bool peano4::grid::internal::twoEventsOverlap(
  const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs
) {
  bool twoEventsOverlap = true;
  for (int d = 0; d < Dimensions; d++) {
    twoEventsOverlap &= lhs.getOffset()(d) + lhs.getWidth()(d) >= rhs.getOffset()(d);
    twoEventsOverlap &= lhs.getOffset()(d) <= rhs.getOffset()(d) + rhs.getWidth()(d);
  }
  return twoEventsOverlap;
};

bool peano4::grid::internal::equals(
  const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs
) {
  bool sameType
    = (lhs.getRefinementControl() == GridControlEvent::RefinementControl::Erase
       and rhs.getRefinementControl() == GridControlEvent::RefinementControl::Erase)
      or (lhs.getRefinementControl() == GridControlEvent::RefinementControl::Refine and rhs.getRefinementControl() == GridControlEvent::RefinementControl::Refine);

  return sameType and tarch::la::equals(lhs.getOffset(), rhs.getOffset())
         and tarch::la::equals(lhs.getWidth(), rhs.getWidth()) and tarch::la::equals(lhs.getH(), rhs.getH());
};

bool peano4::grid::internal::refinementEventOverrulesCoarsening(
  const peano4::grid::GridControlEvent& refineEvent, const peano4::grid::GridControlEvent& eraseEvent
) {
  return twoEventsOverlap(refineEvent, eraseEvent)
         and refineEvent.getRefinementControl() == GridControlEvent::RefinementControl::Refine
         and eraseEvent.getRefinementControl() == GridControlEvent::RefinementControl::Erase
         and tarch::la::oneSmallerEquals(1.0 / 3.0 * refineEvent.getH(), eraseEvent.getH());
};

bool peano4::grid::internal::twoEventsAreAdjacent(
  const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs, double tolerance
) {
  tarch::la::Vector<Dimensions, double> boundingEventOffset = tarch::la::min(lhs.getOffset(), rhs.getOffset());
  tarch::la::Vector<Dimensions, double>
    boundingEventSize = tarch::la::max(lhs.getOffset() + lhs.getWidth(), rhs.getOffset() + rhs.getWidth())
                        - boundingEventOffset;

  const double relativeTolerance = tarch::la::relativeEps(
    tarch::la::volume(lhs.getWidth()), tarch::la::volume(rhs.getWidth()), tolerance
  );
  const double fusedEventsSize = tarch::la::volume(lhs.getWidth()) + tarch::la::volume(rhs.getWidth());
  return tarch::la::equals(lhs.getH(), rhs.getH(), relativeTolerance)
         and tarch::la::smallerEquals(tarch::la::volume(boundingEventSize), fusedEventsSize, relativeTolerance);
};

peano4::grid::GridControlEvent peano4::grid::internal::createBoundingBoxEvent(
  const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs
) {
  tarch::la::Vector<Dimensions, double> boundingEventOffset = tarch::la::min(lhs.getOffset(), rhs.getOffset());
  tarch::la::Vector<Dimensions, double>
    boundingEventSize = tarch::la::max(lhs.getOffset() + lhs.getWidth(), rhs.getOffset() + rhs.getWidth())
                        - boundingEventOffset;
  return GridControlEvent(rhs.getRefinementControl(), boundingEventOffset, boundingEventSize, rhs.getH());
};

void peano4::grid::reduceGridControlEvents([[maybe_unused]] std::vector<GridControlEvent>& events) {
#ifdef Parallel
  std::vector<GridControlEvent> localEvents = events;
  int                           localSize   = events.size();
  int*                          sizes       = new int[tarch::mpi::Rank::getInstance().getNumberOfRanks()];

  logDebug("reduceGridControlEvents(...)", "feed local data of size " << localEvents.size() << " into allgather");

  MPI_Allgather(&localSize, 1, MPI_INT, sizes, 1, MPI_INT, tarch::mpi::Rank::getInstance().getCommunicator());

  int totalSize = 0;
  for (int i = 0; i < tarch::mpi::Rank::getInstance().getNumberOfRanks(); i++) {
    totalSize += sizes[i];
  }
  events.resize(totalSize);
  logDebug(
    "reduceGridControlEvents(...)",
    "we have "
      << totalSize << " grid control event(s) over all ranks, while the local rank hosts " << events.size()
      << " event(s)"
  );

  int offset = 0;
  for (int i = 0; i < tarch::mpi::Rank::getInstance().getNumberOfRanks(); i++) {
    if (i == tarch::mpi::Rank::getInstance().getRank()) {
      for (int j = 0; j < sizes[i]; j++) {
        events[offset + j] = localEvents[j];
      }
    }

    if (sizes[i] > 0) {
      MPI_Bcast(
        events.data() + offset,
        sizes[i],
        GridControlEvent::getGlobalCommunciationDatatype(),
        i,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      offset += sizes[i];
    }
  }

  delete[] sizes;
#endif
}
