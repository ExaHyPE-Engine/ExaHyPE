// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlService.h"

#include "exahype2/RefinementControl.h"
#include "peano4/Globals.h"
#include "tarch/Termination.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Core.h"
#include "tarch/services/ServiceRepository.h"

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

int exahype2::RefinementControlService::_reductionTag =
  tarch::mpi::Rank::reserveFreeTag("exahype2::RefinementControlService");

exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService result;
  return result;
}

exahype2::RefinementControlService::RefinementControlService() :
  _topologyChangedSinceLastTimeStep(false),
  _domainOffset(0.0),
  _numberOfSolvers(1) {
  // One accumulation buffer per thread so merge() needs no lock. The buffers are sized once
  // here. A singleton constructed before tarch::multicore::Core reports its thread count
  // would see zero, which the lower bound turns into the one buffer a serial run needs.
  _localNewEventsPerThread.resize(std::max(tarch::multicore::Core::getInstance().getNumberOfThreads(), 1));
  tarch::services::ServiceRepository::getInstance().addService(this, "exahype2::RefinementControlService");

#ifdef PEANO_WITH_MPI
  _recvSizeRequest = MPI_REQUEST_NULL;
  _isShuttingDown  = false;
  postReceiveSize();
#endif
}

exahype2::RefinementControlService::~RefinementControlService() {
#ifdef PEANO_WITH_MPI
  int mpiFinalised = 0;
  MPI_Finalized(&mpiFinalised);
  if (not mpiFinalised) {
    shutdown();
  }
#endif
  tarch::services::ServiceRepository::getInstance().removeService(this);
}

void exahype2::RefinementControlService::shutdown() {
#ifdef PEANO_WITH_MPI
  if (_isShuttingDown) {
    return;
  }

  freeAllPendingSendRequests();

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

  // Finish every outstanding receive so nothing is left in the queue at finalize, then stop re-posting
  // and cancel the persistent size receive.
  drainDanglingMessages();
  _isShuttingDown = true;
  cancelReceiveSizeRequest();
#endif
}

#ifdef PEANO_WITH_MPI
void exahype2::RefinementControlService::postReceiveSize() {
  if (not _isShuttingDown and _recvSizeRequest == MPI_REQUEST_NULL) {
    tarch::mpi::irecv(
      &_recvSizeBuffer,
      1,
      MPI_INT,
      MPI_ANY_SOURCE,
      _reductionTag,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      &_recvSizeRequest
    );
  }
}

void exahype2::RefinementControlService::tryToReceiveDanglingMessage() {
  // Complete any finished body receives. Each in-flight body is an independent, source-specific irecv,
  // so a slow/large body from one rank never blocks receiving from another - no head-of-line blocking.
  for (auto body = _incomingBodies.begin(); body != _incomingBodies.end();) {
    int flag = 0;
    tarch::mpi::test(&body->request, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      for (const auto& request : body->buffer) {
        _remoteNewEvents.push_back(request);
      }
      body = _incomingBodies.erase(body);
    } else {
      ++body;
    }
  }

  // Drain every pending size header on the persistent ANY_SOURCE receive.
  // For each header post a source-specific body receive and then re-post
  // the size receive immediately, so the size channel never blocks behind a
  // body. The body irecv is posted before the size re-post, so MPI matches the incoming body to it
  // rather than to the (later-posted) size receive.
  postReceiveSize();
  int        flag = 0;
  MPI_Status status;
  tarch::mpi::test(&_recvSizeRequest, &flag, &status);
  while (flag) {
    _recvSizeRequest     = MPI_REQUEST_NULL;
    const int eventCount = _recvSizeBuffer;
    if (eventCount > 0) {
      _incomingBodies.emplace_back();
      IncomingBody& body = _incomingBodies.back();
      body.buffer.resize(eventCount);
      tarch::mpi::irecv(
        body.buffer.data(),
        static_cast<int>(eventCount * sizeof(RawRequest)),
        MPI_BYTE,
        status.MPI_SOURCE,
        _reductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &body.request
      );
    }
    postReceiveSize();
    flag = 0;
    tarch::mpi::test(&_recvSizeRequest, &flag, &status);
  }
}

void exahype2::RefinementControlService::drainDanglingMessages() {
  // Shutdown helper: receive everything still pending, then block-complete all in-flight bodies so no
  // request crosses MPI_Finalize.
  tryToReceiveDanglingMessage();
  for (auto& body : _incomingBodies) {
    tarch::mpi::wait(&body.request, []() {}, MPI_STATUS_IGNORE);
    for (const auto& request : body.buffer) {
      _remoteNewEvents.push_back(request);
    }
  }
  _incomingBodies.clear();
}

void exahype2::RefinementControlService::cancelReceiveSizeRequest() {
  if (_recvSizeRequest != MPI_REQUEST_NULL) {
    MPI_Status status;
    tarch::mpi::cancel(&_recvSizeRequest);
    tarch::mpi::wait(&_recvSizeRequest, []() {}, &status);

    int wasCancelled = 0;
    MPI_Test_cancelled(&status, &wasCancelled);
    _recvSizeRequest = MPI_REQUEST_NULL;

    if (not wasCancelled and _recvSizeBuffer > 0) {
      // A header arrived just before the cancel; receive and process its body so the sender's matching
      // body send completes and nothing is left in the queue.
      std::vector<RawRequest> body(_recvSizeBuffer);
      MPI_Request             bodyRequest = MPI_REQUEST_NULL;
      tarch::mpi::irecv(
        body.data(),
        static_cast<int>(_recvSizeBuffer * sizeof(RawRequest)),
        MPI_BYTE,
        status.MPI_SOURCE,
        _reductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &bodyRequest
      );
      tarch::mpi::wait(&bodyRequest, []() {}, MPI_STATUS_IGNORE);
      for (const auto& request : body) {
        _remoteNewEvents.push_back(request);
      }
    }
  }
}
#endif

/**
 * Receive refinement control messages from other ranks
 *
 * Incoming messages carry the raw requests a partner rank collected, so they join
 * _remoteNewEvents. A finishStep() that exchanges collectively drops them, one that does
 * not folds them into its raw request set. They must not shortcut into _committedEvents:
 * the sibling-group promotion has to see them as raw requests, and the running traversal
 * reads the committed set.
 *
 * The service needs no lock. tarch::services::ServiceRepository::receiveDanglingMessages()
 * serialises the polling, so only one thread probes and receives at a time; the trees only
 * read _committedEvents, which finishStep() alone writes and only in between traversals;
 * and merge() writes one buffer per thread. _remoteNewEvents is the one member both the
 * poller and finishStep() touch, and the repository lock does not cover finishStep(). It
 * is safe because finishStep() runs single-threaded between traversals, and the polling it
 * re-enters while waiting for its own collective happens on that same thread.
 */
void exahype2::RefinementControlService::receiveDanglingMessages() {
#ifdef PEANO_WITH_MPI
  tryToReceiveDanglingMessage();
#endif
}

std::string exahype2::RefinementControlService::toString() const {
  std::size_t numberOfLocalEvents = 0;
  for (const auto& threadBuffer : _localNewEventsPerThread) {
    numberOfLocalEvents += threadBuffer.size();
  }
  std::ostringstream msg;
  msg << "("
      << "#new-local-events=" << numberOfLocalEvents << "#new-remote-events=" << _remoteNewEvents.size()
      << ",#committed-events=" << _committedEvents.size() << ")";
  return msg.str();
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
 * The raw requests are exchanged via MPI before the coarsening consensus runs.
 * The default path first waits for all previous non-blocking communication to
 * terminate (freeAllPendingSendRequests()), creates a copy of the raw requests,
 * and then delegates the data exchange to triggerSendOfCopyOfRawRequests(); the
 * requests other ranks sent us join the set afterwards. If useCollectiveExchange
 * is set, all ranks synchronise their requests through one blocking collective
 * exchange, which is what lets a sibling group split across ranks be reassembled.
 *
 * The request set is promoted and merged here and not per tree: Peano's spacetrees
 * would otherwise each redo the same work, and with a hundred trees per node that
 * is time better spent elsewhere.
 *
 * @param numberOfDeferredTopologyChanges Vertex changes the completed traversal
 *   requested but could not apply; a positive count re-delivers the traversal's
 *   event set (see finishDynamicMeshRefinementStep()).
 */
void exahype2::RefinementControlService::finishStep(bool useCollectiveExchange, int numberOfDeferredTopologyChanges) {
  finishStep(useCollectiveExchange, false, false, numberOfDeferredTopologyChanges);
}

/**
 * Commit the events collected by a refinement criterion traversal
 *
 * If the merged event set is identical to the last event set that a mesh
 * refinement cycle fully realised, the events are dropped. Reissuing the same
 * geometric commands cannot change the mesh again, but would otherwise force two
 * expensive AMR realisation traversals after every time step.
 */
void exahype2::RefinementControlService::finishRefinementCriterionStep(bool useCollectiveExchange) {
  // The completed traversal is a time step: its roll-over has compensated any
  // Updated entries the preceding mesh-update sweeps cleared.
  _topologyChangedSinceLastTimeStep = false;
  finishStep(useCollectiveExchange, true, false);
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
void exahype2::RefinementControlService::finishDynamicMeshRefinementStep(
  bool useCollectiveExchange,
  int  numberOfDeferredTopologyChanges
) {
  // The realise sweeps re-evaluate the refinement criterion on cells they
  // created, so a drain keeps committing until the mesh satisfies it.
  // Suppressing sets the mesh already realised is what ends the drain; batches
  // with pending realisation deliveries are exempt from the suppression.
  finishStep(useCollectiveExchange, true, true, numberOfDeferredTopologyChanges);
}

void exahype2::RefinementControlService::finishStep(
  bool useCollectiveExchange,
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
    logDebug(
      "Activate " << rawRequests.size() << " local refinement/erase instruction(s) for the next grid sweep, plus "
                  << _remoteNewEvents.size() << " request(s) received from other ranks"
    );
  }

#ifdef PEANO_WITH_MPI
  freeAllPendingSendRequests();
  if (useCollectiveExchange) {
    // The collective exchange leaves every rank with the same set. Point-to-point requests
    // that arrived in the meantime stem from an earlier sweep and would break that, so they go.
    _remoteNewEvents.clear();
    exchangeRawRequestsCollectively(rawRequests);
    // A change deferred on any rank keeps the events alive on every rank, so the redelivery
    // decision below is rank-uniform. The maximum expresses "deferred anywhere" and, other
    // than a sum, cannot overflow at scale.
    int globalDeferredTopologyChanges = 0;
    MPI_Allreduce(
      &numberOfDeferredTopologyChanges,
      &globalDeferredTopologyChanges,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    numberOfDeferredTopologyChanges = globalDeferredTopologyChanges;
  } else {
    _copyOfRawRequests = rawRequests;
    triggerSendOfCopyOfRawRequests();
  }
#endif

  if (not useCollectiveExchange) {
    for (const auto& remoteRequest : _remoteNewEvents) {
      rawRequests.push_back(remoteRequest);
    }
  }
  // The collective above waits through the service poller, so further point-to-point requests
  // can have landed since the clear before it. They belong to the same rank-uniform set the
  // exchange already established and go the same way.
  _remoteNewEvents.clear();

  // On the collective path every rank holds the same request set, so all ranks reach the same
  // suppression and redelivery decisions; a commit without that exchange decides from its own
  // requests.
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
    // a pass-through event, and after a collective exchange every rank stages the identical
    // parent, so the merge deduplicates the copies.
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

#ifdef PEANO_WITH_MPI
/**
 * Synchronise the raw requests across all ranks in one blocking exchange
 *
 * Every rank contributes its request set and receives the concatenation of all
 * ranks' sets, so after the call each rank holds an identical set. The mesh
 * refinement realisation traversals use this deterministic path; the asynchronous
 * point-to-point path serves the per-time-step commits, which tolerate a one-sweep
 * delay.
 */
void exahype2::RefinementControlService::exchangeRawRequestsCollectively(std::vector<RawRequest>& requests) {
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

void exahype2::RefinementControlService::freeAllPendingSendRequests() {
  for (MPI_Request* request : _sendRequests) {
    if (request != nullptr) {
      tarch::mpi::wait(
        request,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); },
        MPI_STATUS_IGNORE
      );
      delete request;
    }
  }
  _sendRequests.clear();
}

/**
 * Send a copy of the raw requests to every other rank
 *
 * The MPI data exchange here is non-trivial, as we do not know which rank has how
 * many messages. So we run in three steps: First, we send the raw requests out to
 * all partners. These sends are done non-blocking. They logically are a broadcast.
 * Second, we loop over all the arising MPI_Requests and poll until they are finished.
 * So we know all data has gone out. This is a while loop with the predicate
 *
 *       not sendRequests.empty()
 *
 * Third, we hook into this while loop and probe for all the other ranks if they
 * have sent us anything. So while we wait for our sends to go out, we poll the other
 * ranks for their data, integrate this information into our data structures and
 * therefore also free any MPI queues.
 *
 * As we build up the incoming data while we wait for our sends to go out, we have to
 * send from a copy of the actual data (_copyOfRawRequests): We cannot alter the
 * outgoing data due to polls for incoming messages while we have still pending sends.
 *
 * We have to work with non-blocking calls here, as the code otherwise tends to
 * deadlock if we have a lot of events. This is due to MPI falling back to rendezvous
 * message passing if the data that is on-the-fly becomes too large.
 */
void exahype2::RefinementControlService::triggerSendOfCopyOfRawRequests() {
  assertion(_sendRequests.empty());

  logDebug("Share the local " << _copyOfRawRequests.size() << " request(s) with the other ranks");
  if (_copyOfRawRequests.size() > 0) {
    // Per peer: a small size header (so the receiver can size its body irecv without an ANY_SOURCE
    // probe) followed by the body. The receiver re-posts the size receive immediately and runs body
    // receives concurrently, so this does not head-of-line-block at scale.
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    _sendSizeBuffers.resize(ranks);

    for (int rank = 0; rank < ranks; rank++) {
      if (rank != tarch::mpi::Rank::getInstance().getRank()) {
        _sendSizeBuffers[rank] = static_cast<int>(_copyOfRawRequests.size());
        MPI_Request* sizeReq   = new MPI_Request();
        tarch::mpi::isend(
          &_sendSizeBuffers[rank],
          1,
          MPI_INT,
          rank,
          _reductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          sizeReq
        );
        _sendRequests.push_back(sizeReq);

        MPI_Request* bodyReq = new MPI_Request();
        tarch::mpi::isend(
          _copyOfRawRequests.data(),
          static_cast<int>(_copyOfRawRequests.size() * sizeof(RawRequest)),
          MPI_BYTE,
          rank,
          _reductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          bodyReq
        );
        _sendRequests.push_back(bodyReq);
      }
    }
  }
}
#endif

std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents() const {
  return _committedEvents;
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
