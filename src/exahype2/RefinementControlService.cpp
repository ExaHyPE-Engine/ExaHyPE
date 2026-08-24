// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlService.h"

#include "exahype2/RefinementControl.h"
#include "peano4/Globals.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Termination.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

namespace {
  /**
   * Names the Peano parent cell a leaf-sized erase request belongs to.
   *
   * The parent index is an integer, so every sibling of a group produces the same key no
   * matter which tree or rank raised the request. targetH separates the levels, whose parent
   * indices otherwise share the same small numbers.
   */
  struct EraseParentKey {
    std::array<long long, DIMENSIONS> parentIndex{};
    std::array<double, DIMENSIONS>    targetH{};

    bool operator<(const EraseParentKey& other) const noexcept {
      if (parentIndex != other.parentIndex) {
        return parentIndex < other.parentIndex;
      }
      return targetH < other.targetH;
    }
  };

  struct EraseSiblingGroup {
    peano4::grid::GridControlEvent                     parentEvent;
    std::map<std::int32_t, std::bitset<THREE_POWER_D>> childrenPerSolver;
    bool                                               hasFirstDeliveryConstituent = false;
  };

  long long floorDivideByThree(long long value) noexcept { return value >= 0 ? value / 3 : -((-value + 2) / 3); }

  bool isOneLevelCoarseningEvent(const peano4::grid::GridControlEvent& event) noexcept {
    return 3.0 * event.width == event.targetH;
  }

  /**
   * Map a leaf-sized erase request onto its Peano parent cell.
   *
   * Cells sit on a triadic grid anchored at the domain offset, so dividing a cell's offset by
   * its width yields its index on its own level up to a roundoff far below one half, and
   * rounding recovers the index exactly. Dividing that index by three names the parent and the
   * child's slot within the parent's 3^d sibling group. Requests whose siblings spread over
   * two parents therefore land in different groups and no group ever fills up, which is what
   * makes a shifted block of erase requests coarsen nothing.
   */
  bool createEraseParent(
    const peano4::grid::GridControlEvent&        event,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    peano4::grid::GridControlEvent&              parentEvent,
    EraseParentKey&                              key,
    int&                                         linearisedChild
  ) {
    parentEvent       = event;
    parentEvent.width = event.targetH;
    linearisedChild   = 0;
    int stride        = 1;

    for (int d = 0; d < DIMENSIONS; d++) {
      if (event.width[d] <= 0.0 or event.targetH[d] <= 0.0) {
        return false;
      }

      const long long cellIndex   = std::llround((event.offset[d] - domainOffset[d]) / event.width[d]);
      const long long parentIndex = floorDivideByThree(cellIndex);
      const int       child       = static_cast<int>(cellIndex - 3 * parentIndex);

      parentEvent.offset[d] = domainOffset[d] + static_cast<double>(parentIndex) * event.targetH[d];
      key.parentIndex[d]    = parentIndex;
      key.targetH[d]        = event.targetH[d];

      linearisedChild += child * stride;
      stride *= 3;
    }

    return true;
  }
} // namespace

exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService result;
  return result;
}

exahype2::RefinementControlService::RefinementControlService() :
  _topologyChangedSinceLastTimeStep(false),
  _lastCriterionStepWasSuppressed(false),
  _domainOffset(0.0),
  _numberOfSolvers(1) {
  // One accumulation buffer per thread so merge() needs no lock. The buffers are sized once
  // here. A singleton constructed before tarch::multicore::Core reports its thread count
  // would see zero, which the lower bound turns into the one buffer a single-threaded run needs.
  _localNewEventsPerThread.resize(std::max(tarch::multicore::Core::getInstance().getNumberOfThreads(), 1));
}

/**
 * Report how many requests and events the service holds
 *
 * The three counts follow one request through its stages: it accumulates in a per-thread
 * buffer, becomes a committed event for the next traversal, and ends up in the set that
 * the last refinement cycle realised. The solver count is the consent a coarsening needs,
 * so a wrong count shows up here as coarsening that never happens.
 */
std::string exahype2::RefinementControlService::toString() const {
  std::size_t numberOfNewLocalEvents = 0;
  for (const auto& threadBuffer : _localNewEventsPerThread) {
    numberOfNewLocalEvents += threadBuffer.size();
  }

  std::ostringstream out;
  out << "RefinementControlService{";
  out << "#NewLocalEvents=" << numberOfNewLocalEvents;
  out << ", #CommittedEvents=" << _committedEvents.size();
  out << ", #LastRealisedEvents=" << _lastRealisedEvents.size();
  out << ", TopologyChangedSinceLastTimeStep=" << (_topologyChangedSinceLastTimeStep ? "true" : "false");
  out << ", #Solvers=" << _numberOfSolvers;
  out << "}";
  return out.str();
}

void exahype2::RefinementControlService::merge(const RefinementControl& control) {
  // Lock-free: merge() is called once per tree at end-of-traversal from concurrent
  // multicore tasks, and each thread appends into its own buffer.
  const int threadNumber = tarch::multicore::Core::getInstance().getThreadNumber();
  assertion(
    threadNumber >= 0 and threadNumber < static_cast<int>(_localNewEventsPerThread.size()),
    threadNumber,
    _localNewEventsPerThread.size()
  );
  auto& localBuffer = _localNewEventsPerThread[threadNumber];
  localBuffer.insert(localBuffer.end(), control._newEvents.begin(), control._newEvents.end());
}

/**
 * Reduce the requests of one completed traversal into the committed event set
 *
 * This is called after each traversal per rank. It reduces the per-thread request
 * buffers into one raw request set and derives _committedEvents from it, the events
 * active for the next mesh traversal. Every trigger-phase request is staged once
 * more, so Peano can realise the topology transition the next traversal triggers;
 * after that realisation delivery, the request is gone.
 *
 * The raw requests are exchanged via MPI before the coarsening consensus runs, so
 * a sibling group whose children are spread over several ranks is reassembled on
 * each of them.
 *
 * The request set is promoted and merged here and not per tree: Peano's spacetrees
 * would otherwise each redo the same work, and with a hundred trees per node that
 * is time better spent elsewhere.
 *
 * @param numberOfDeferredTopologyChanges Vertex changes the completed traversal
 *   requested but could not apply; a positive count re-delivers the traversal's
 *   event set (see finishDynamicMeshRefinementStep()).
 */
void exahype2::RefinementControlService::finishStep(int numberOfDeferredTopologyChanges) {
  finishStep(false, false, numberOfDeferredTopologyChanges);
}

/**
 * Commit the events collected by a refinement criterion traversal
 *
 * If the merged event set is identical to the last event set that a mesh
 * refinement cycle fully realised, the events are dropped. Reissuing the same
 * geometric commands cannot change the mesh again, but would otherwise force two
 * expensive AMR realisation traversals after every time step.
 */
void exahype2::RefinementControlService::finishRefinementCriterionStep() {
  // The completed traversal is a time step: its roll-over has compensated any
  // Updated entries the preceding mesh-update sweeps cleared. Whether the
  // criterion was silenced during it outlives the flag, because the empty set it
  // then produced must not be mistaken for a settled mesh.
  _lastCriterionStepWasSuppressed   = _topologyChangedSinceLastTimeStep;
  _topologyChangedSinceLastTimeStep = false;
  finishStep(true, false);
}

/**
 * Finish a mesh refinement realisation traversal
 *
 * @param numberOfDeferredTopologyChanges Vertex changes the completed traversal
 *   requested but could not apply (peano4::grid::GridStatistics). A positive count
 *   keeps the traversal's events alive for another delivery; only a traversal that
 *   deferred nothing and left no follow-up event turns its event set into the last
 *   fully realised set.
 *
 * An erase the traversal vetoed, for example against the two-to-one constraint,
 * does not keep the set alive: the veto depends only on the committed events and
 * the topology they realised, so redelivering an identical set reproduces it,
 * while a mesh change that clears it also changes the cell-sized events the
 * criterion emits, which escapes the suppression on its own. Blocking the
 * recording on a veto would keep an identical, structurally vetoed request alive
 * forever, and a caller that repeats mesh updates until the suppression reports
 * the mesh realised would never terminate.
 */
void exahype2::RefinementControlService::finishDynamicMeshRefinementStep(int numberOfDeferredTopologyChanges) {
  // The realise sweeps re-evaluate the refinement criterion on cells they
  // created, so a drain keeps committing until the mesh satisfies it.
  // Suppressing sets the mesh already realised is what ends the drain; batches
  // with pending realisation deliveries are exempt from the suppression.
  finishStep(true, true, numberOfDeferredTopologyChanges);
}

void exahype2::RefinementControlService::finishStep(
  bool suppressEventsIfUnchanged,
  bool recordRealisedEvents,
  int  numberOfDeferredTopologyChanges
) {
  // No lock: finishStep() runs from the main loop after the mesh traversal has joined,
  // i.e. single-threaded. The per-thread merge() buffers are stable here and reduced
  // into the raw request set below. The set the traversal just ran on is already finalised
  // and merged, so it is the canonical form the suppression check compares against.
  const std::vector<peano4::grid::GridControlEvent> eventsUsedByCompletedTraversal = std::move(_committedEvents);
  _committedEvents.clear();

  if (recordRealisedEvents and not eventsUsedByCompletedTraversal.empty()) {
    _topologyChangedSinceLastTimeStep = true;
  }

  // Trigger-phase requests are kept aside per event: each needs one more delivery, so Peano
  // can realise the topology change the next traversal triggers. Only the rank that raised
  // a request re-delivers it; remote ranks receive the re-delivery through the next exchange.
  std::vector<RawRequest>                  rawRequests;
  std::vector<RefinementControl::NewEvent> localTriggerRequests;
  for (auto& threadBuffer : _localNewEventsPerThread) {
    for (const auto& newEvent : threadBuffer) {
      const bool isFirstDelivery =
        newEvent.phase == RefinementControl::NewEvent::RealisationPhase::TriggerTopologyChange;
      rawRequests.push_back(
        RawRequest{newEvent.event, static_cast<std::int32_t>(newEvent.solverNumber), isFirstDelivery ? 1 : 0}
      );

      if (isFirstDelivery) {
        localTriggerRequests.push_back(newEvent);
      }
    }
    threadBuffer.clear();
  }

  if (not rawRequests.empty()) {
    logDebug("Activate " << rawRequests.size() << " local refinement/erase instruction(s) for the next grid sweep");
  }

  exchangeRawRequests(rawRequests);

  // A change deferred on any rank keeps the events alive on every rank, so the redelivery
  // decision below is rank-uniform. The maximum expresses "deferred anywhere" and, other
  // than a sum, cannot overflow at scale.
  int globalDeferredTopologyChanges = 0;
  tarch::mpi::allreduce(
    &numberOfDeferredTopologyChanges,
    &globalDeferredTopologyChanges,
    1,
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  numberOfDeferredTopologyChanges = globalDeferredTopologyChanges;

  // Every rank holds the same request set, so all ranks reach the same suppression and
  // redelivery decisions.
  const bool hasRealiseRequests = std::any_of(rawRequests.begin(), rawRequests.end(), [](const RawRequest& request) {
    return request.isFirstDelivery == 0;
  });

  // Both the consensus promotion and the merge are deterministic, so a rank fed the same raw
  // requests produces a bitwise identical committed set. The promotion leaves every erase
  // event as one whole parent cell, which is the precondition for fusing erase events.
  std::vector<peano4::grid::GridControlEvent> parentEventsRequiringRealisationDelivery;
  _committedEvents = finaliseCoarseningEvents(rawRequests, parentEventsRequiringRealisationDelivery);
  peano4::grid::GridControlEvent::mergeCellAlignedEvents(_committedEvents);

  // A batch that still carries realise-phase requests is never suppressed: dropping it would
  // rob the previous trigger delivery of its realisation traversal.
  if (suppressEventsIfUnchanged and not hasRealiseRequests and _committedEvents == _lastRealisedEvents) {
    logDebug("Drop " << _committedEvents.size() << " already realised refinement/erase instruction(s)");
    _committedEvents.clear();
  } else {
    // Re-deliver every local trigger request next sweep to realise the topology change. This
    // runs single-threaded, so the requests are staged in the thread-0 buffer; the next
    // finishStep() reduces all per-thread buffers regardless of origin. Staging the raw
    // requests keeps solver identity and sibling groups intact, so the next consensus pass
    // reproduces the same committed set.
    for (const auto& request : localTriggerRequests) {
      _localNewEventsPerThread[0].push_back(
        RefinementControl::NewEvent{
          request.event,
          RefinementControl::NewEvent::RealisationPhase::RealiseTopologyChange,
          request.solverNumber
        }
      );
    }
    // A parent that has just formed from a group with spent constituent deliveries still
    // needs its realisation traversal: stage it as a whole. It re-enters the next batch as
    // a pass-through event, and every rank stages the identical parent, so the merge
    // deduplicates the copies.
    for (const auto& parentEvent : parentEventsRequiringRealisationDelivery) {
      _localNewEventsPerThread[0].push_back(
        RefinementControl::NewEvent{
          parentEvent,
          RefinementControl::NewEvent::RealisationPhase::RealiseTopologyChange,
          0
        }
      );
    }
  }

  if (numberOfDeferredTopologyChanges > 0 and not eventsUsedByCompletedTraversal.empty()) {
    // The completed traversal could not apply every requested vertex change: deliver its
    // event set again, so the deferred changes are retried once the neighbouring
    // transitions have settled. The set is already canonical, so the merge only removes
    // duplicates against the new batch.
    _committedEvents
      .insert(_committedEvents.end(), eventsUsedByCompletedTraversal.begin(), eventsUsedByCompletedTraversal.end());
    peano4::grid::GridControlEvent::mergeCellAlignedEvents(_committedEvents);
  }

  if (recordRealisedEvents and numberOfDeferredTopologyChanges == 0 and _committedEvents.empty()
      and not eventsUsedByCompletedTraversal.empty()) {
    _lastRealisedEvents = eventsUsedByCompletedTraversal;
  }
}

/**
 * Promote patch-local erase requests to parent-sized erase events
 *
 * Every one-level erase request is mapped onto its Peano parent cell; a parent-sized
 * event survives only if every solver has requested the erase for the complete 3^d
 * sibling group. Incomplete groups are dropped, so a single smooth patch can never
 * coarsen away siblings that still carry a feature, and a solver that keeps or skips
 * any sibling vetoes the coarsening for all solvers. This is where a coarsening
 * decision is made; the subsequent merge only fuses boxes.
 *
 * A parent formed with any first-delivery constituent still needs the realisation
 * delivery of the following sweep and is reported through the second parameter, so
 * the caller can stage it once more even though its constituents' own deliveries
 * may already be spent.
 */
std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::finaliseCoarseningEvents(
  const std::vector<RawRequest>&               requests,
  std::vector<peano4::grid::GridControlEvent>& parentEventsRequiringRealisationDelivery
) const {
  std::map<EraseParentKey, EraseSiblingGroup> siblingGroups;
  std::vector<peano4::grid::GridControlEvent> finalisedEvents;
  finalisedEvents.reserve(requests.size());

  for (const auto& request : requests) {
    const auto& event = request.event;
    if (event.getRefinementControl() != peano4::grid::GridControlEvent::RefinementControl::Erase
        or not isOneLevelCoarseningEvent(event)) {
      finalisedEvents.push_back(event);
      continue;
    }

    peano4::grid::GridControlEvent parentEvent;
    EraseParentKey                 key;
    int                            linearisedChild = 0;
    if (createEraseParent(event, _domainOffset, parentEvent, key, linearisedChild)) {
      auto& group = siblingGroups[key];
      if (group.childrenPerSolver.empty()) {
        group.parentEvent = parentEvent;
      }
      group.childrenPerSolver[request.solverNumber].set(linearisedChild);
      group.hasFirstDeliveryConstituent = group.hasFirstDeliveryConstituent or request.isFirstDelivery != 0;
    }
  }

  for (const auto& [key, group] : siblingGroups) {
    // Coarsening is unanimous: every solver has to request the complete sibling group. A
    // solver that keeps any sibling, or has not evaluated its criterion in this sweep,
    // therefore vetoes the erase for all solvers.
    bool everySolverAgrees = static_cast<int>(group.childrenPerSolver.size()) == _numberOfSolvers;
    for (const auto& [solverNumber, children] : group.childrenPerSolver) {
      everySolverAgrees = everySolverAgrees and children.count() == THREE_POWER_D;
    }
    if (everySolverAgrees) {
      finalisedEvents.push_back(group.parentEvent);
      if (group.hasFirstDeliveryConstituent) {
        parentEventsRequiringRealisationDelivery.push_back(group.parentEvent);
      }
    }
  }

  return finalisedEvents;
}

/**
 * Replace the local raw requests by the union over all ranks
 *
 * Every rank contributes its request set and receives the concatenation of all
 * ranks' sets, so after the call each rank holds an identical set and every
 * decision derived from it is rank-uniform.
 */
void exahype2::RefinementControlService::exchangeRawRequests(std::vector<RawRequest>& requests) {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  // The classic collective interface carries int counts. Exchanging whole requests
  // through a committed contiguous type instead of bytes raises the ceiling to 2^31
  // requests per rank and aggregate; the guards turn a breach into a hard error
  // instead of silent truncation.
  static MPI_Datatype rawRequestType = MPI_DATATYPE_NULL;
  if (rawRequestType == MPI_DATATYPE_NULL) {
    MPI_Type_contiguous(static_cast<int>(sizeof(RawRequest)), MPI_BYTE, &rawRequestType);
    MPI_Type_commit(&rawRequestType);
  }

  if (requests.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    tarch::terminateWithError(
      "The local refinement-request count exceeds the collective exchange's representable range."
    );
  }
  const int localRequestCount = static_cast<int>(requests.size());

  std::vector<int> requestCounts(numberOfRanks, 0);
  tarch::mpi::allgather(
    &localRequestCount,
    1,
    MPI_INT,
    requestCounts.data(),
    1,
    MPI_INT,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  std::vector<int> displacements(numberOfRanks, 0);
  std::size_t      totalRequestCount = requestCounts[0];
  for (int rank = 1; rank < numberOfRanks; ++rank) {
    displacements[rank] = displacements[rank - 1] + requestCounts[rank - 1];
    totalRequestCount += requestCounts[rank];
  }
  if (totalRequestCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    tarch::terminateWithError(
      "The global refinement-request count exceeds the collective exchange's representable range."
    );
  }

  std::vector<RawRequest> gatheredRequests(totalRequestCount);
  tarch::mpi::allgatherv(
    localRequestCount == 0 ? nullptr : requests.data(),
    localRequestCount,
    rawRequestType,
    totalRequestCount == 0 ? nullptr : gatheredRequests.data(),
    requestCounts.data(),
    displacements.data(),
    rawRequestType,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  requests = std::move(gatheredRequests);
}

std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents() const {
  return getGridControlEvents(peano4::parallel::SpacetreeSet::getInstance().getLocalTreeExtents());
}

/**
 * Deliver the committed events a tree within the given extents can realise.
 *
 * The committed set itself is identical on every rank, and the suppression and
 * redelivery decisions rely on that, so it stays global. A cell however only
 * realises an event it overlaps, so the delivery keeps the events that touch a
 * tree's root box, enlarged by one event width so an event on a partition seam
 * reaches both of its sides. Per-tree copies and per-cell scans then scale
 * with the local subdomain instead of the global grid.
 */
std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents(
  const std::vector<peano4::parallel::SpacetreeSet::TreeExtent>& localTreeExtents
) const {
  std::vector<peano4::grid::GridControlEvent> deliveredEvents;
  deliveredEvents.reserve(_committedEvents.size());
  for (const auto& event : _committedEvents) {
    const tarch::la::Vector<DIMENSIONS, double> eventLow  = event.offset - event.width;
    const tarch::la::Vector<DIMENSIONS, double> eventHigh = event.offset + 2.0 * event.width;
    for (const auto& extent : localTreeExtents) {
      bool overlaps = true;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        overlaps = overlaps and eventLow(direction) < extent.offset(direction) + extent.width(direction)
          and extent.offset(direction) < eventHigh(direction);
      }
      if (overlaps) {
        deliveredEvents.push_back(event);
        break;
      }
    }
  }
  return deliveredEvents;
}

/**
 * Is there at least one committed grid-control event for the next mesh traversal?
 *
 * The predicate distinguishes a sweep that produced mesh work from one that did not,
 * so a caller can restrict AMR-only sweeps to the steps that need them. Once such
 * sweeps run, they have to run to completion: Peano needs one traversal to trigger
 * the refinement and another to realise the created and destroyed cells and faces.
 */
bool exahype2::RefinementControlService::hasGridControlEvents() const { return not _committedEvents.empty(); }

/**
 * Has a topology change been realised since the last time step?
 *
 * True while the first time step after a realised topology change runs, so the face
 * roll-over compensates the Updated entries the mesh-update sweeps cleared.
 */
bool exahype2::RefinementControlService::hasRealisedTopologyChangesSinceLastTimeStep() const {
  return _topologyChangedSinceLastTimeStep;
}

/**
 * Did the criterion traversal that just finished collect nothing because the mesh had changed?
 *
 * A caller that keeps updating the mesh until the criterion asks for nothing has to
 * treat such a traversal as undecided. Reading its empty request set as agreement
 * ends the drain one round early and leaves the mesh coarser than the criterion wants.
 */
bool exahype2::RefinementControlService::lastCriterionStepWasSuppressed() const {
  return _lastCriterionStepWasSuppressed;
}

void exahype2::RefinementControlService::setDomainOffset(const tarch::la::Vector<DIMENSIONS, double>& domainOffset) {
  // Set once during setup (single-threaded), so no lock.
  _domainOffset = domainOffset;
}

/**
 * Set the number of solvers whose consent a coarsening requires
 *
 * Coarsening requires the consent of every solver that evaluates a refinement
 * criterion, so the service needs the count of those solvers. Registrations that
 * never submit requests, such as limiting controllers, must not be counted: they
 * can never consent and would veto every coarsening. Defaults to one.
 */
void exahype2::RefinementControlService::setNumberOfSolvers(int numberOfSolvers) {
  assertion(numberOfSolvers >= 1);
  // Set once during setup (single-threaded), so no lock.
  _numberOfSolvers = numberOfSolvers;
}
