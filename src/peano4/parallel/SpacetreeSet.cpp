// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/Node.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"

peano4::parallel::SpacetreeSet peano4::parallel::SpacetreeSet::_singleton;

tarch::multicore::BooleanSemaphore peano4::parallel::SpacetreeSet::_semaphore;

peano4::parallel::SpacetreeSet& peano4::parallel::SpacetreeSet::getInstance() { return _singleton; }

peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::parallel::SpacetreeSet::getLocalExchangeStack(int treeId, int localStackId) {
  return getSpacetree(treeId).getLocalExchangeVertexStack(localStackId);
}

std::vector<peano4::grid::PeriodicStamp>&
peano4::parallel::SpacetreeSet::getLocalExchangeStamps(int treeId, int localStackId) {
  return getSpacetree(treeId).getLocalExchangeVertexStamps(localStackId);
}

void peano4::parallel::SpacetreeSet::pushLocalExchangeStamp(
  int                        treeId,
  int                        localStackId,
  const grid::PeriodicStamp& stamp
) {
  getLocalExchangeStamps(treeId, localStackId).push_back(stamp);
}

void peano4::parallel::SpacetreeSet::stageCrossRankPeriodicEntry(
  int                        sourceTreeId,
  int                        destinationTreeId,
  const grid::PeriodicStamp& stamp,
  int                        destinationStackId,
  grid::GridVertex           vertexData
) {
  _crossRankPeriodicGridVertexStaging.push_back(
    grid::CrossRankPeriodicEntry<grid::GridVertex>{
      sourceTreeId,
      destinationTreeId,
      stamp,
      destinationStackId,
      vertexData
    }
  );
}

std::vector<peano4::grid::CrossRankPeriodicEntry<peano4::grid::GridVertex>>&
peano4::parallel::SpacetreeSet::getCrossRankPeriodicStaging() {
  return _crossRankPeriodicGridVertexStaging;
}

void peano4::parallel::SpacetreeSet::clearCrossRankPeriodicStaging() { _crossRankPeriodicGridVertexStaging.clear(); }

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::parallel::SpacetreeSet::getParallelExchangeStack(
  int                                                treeId,
  int                                                neighbourSlot,
  peano4::grid::StackTopology::ParallelExchangeStack exchangeStack
) {
  return getSpacetree(treeId).getParallelExchangeVertexStack(neighbourSlot, exchangeStack);
}

void peano4::parallel::SpacetreeSet::registerLocalSpacetree(peano4::grid::Spacetree& tree) {
  const int localTreeId = Node::getInstance().getLocalTreeId(tree._id);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  assertion(
    _spacetreeByLocalTreeId[localTreeId] == nullptr,
    "Duplicate local tree id.",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  _spacetreeByLocalTreeId[localTreeId] = &tree;
}

void peano4::parallel::SpacetreeSet::unregisterLocalSpacetree(int treeId) {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  if (localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK) {
    [[maybe_unused]] auto* const tree = _spacetreeByLocalTreeId[localTreeId];
    assertion(
      tree == nullptr or tree->_id == treeId,
      "Spacetree lookup mismatch.",
      treeId,
      (tree == nullptr ? -1 : tree->_id),
      localTreeId
    );
    _spacetreeByLocalTreeId[localTreeId] = nullptr;
  }
}

void peano4::parallel::SpacetreeSet::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  _spacetreeByLocalTreeId.fill(nullptr);

  _requestMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - request message");
  _answerMessageTag =
    tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - answer message", MAX_SPACETREES_PER_RANK);
  _stateMessageTag  = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - automaton state message");
  _treeIdMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - tree id batch");
  tarch::services::ServiceRepository::getInstance().addService(this, "peano4::parallel::SpacetreeSet");

#ifdef PEANO_WITH_MPI
  assertion(
    (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees() == 1
     and tarch::mpi::Rank::getInstance().getRank() == 0)
      or (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees() == 0 and tarch::mpi::Rank::getInstance().getRank() != 0),
    peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees(),
    offset,
    width,
    tarch::mpi::Rank::getInstance().getRank()
  );

  _treeManagementReceiveIsPosted = false;
  postTreeManagementReceive();
#endif

  _periodicBC = periodicBC;

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _spacetrees.emplace_back(0, offset, width, periodicBC);
    registerLocalSpacetree(_spacetrees.back());
  }
}

void peano4::parallel::SpacetreeSet::shutdown() {
#ifdef PEANO_WITH_MPI
  progressPendingTreeManagement();

  for (auto& response : _pendingTreeIdResponses) {
    tarch::mpi::wait(&response.sendRequest, []() {}, MPI_STATUS_IGNORE);
  }
  _pendingTreeIdResponses.clear();

  for (auto& message : _pendingTreeManagementMessages) {
    tarch::mpi::wait(&message.sendRequest, []() {}, MPI_STATUS_IGNORE);
  }
  _pendingTreeManagementMessages.clear();

  for (auto& creation : _pendingTreeCreations) {
    if (creation.treeIdsReceiveRequest != MPI_REQUEST_NULL) {
      tarch::mpi::cancel(&creation.treeIdsReceiveRequest);
      tarch::mpi::wait(&creation.treeIdsReceiveRequest, []() {}, MPI_STATUS_IGNORE);
    }
    if (creation.rootStateReceiveRequest != MPI_REQUEST_NULL) {
      tarch::mpi::cancel(&creation.rootStateReceiveRequest);
      tarch::mpi::wait(&creation.rootStateReceiveRequest, []() {}, MPI_STATUS_IGNORE);
    }
  }
  _pendingTreeCreations.clear();

  if (_treeManagementReceiveIsPosted) {
    tarch::mpi::cancel(&_treeManagementReceiveRequest);
    tarch::mpi::wait(&_treeManagementReceiveRequest, []() {}, MPI_STATUS_IGNORE);
    _treeManagementReceiveRequest  = MPI_REQUEST_NULL;
    _treeManagementReceiveIsPosted = false;
  }
#endif

  tarch::services::ServiceRepository::getInstance().removeService(this);
}

#ifdef PEANO_WITH_MPI
void peano4::parallel::SpacetreeSet::postTreeManagementReceive() {
  if (_treeManagementReceiveIsPosted) {
    return;
  }

  tarch::mpi::irecv(
    &_treeManagementReceiveBuffer,
    1,
    TreeManagementMessage::getJoinDatatype(),
    MPI_ANY_SOURCE,
    _requestMessageTag,
    tarch::mpi::Rank::getInstance().getCommunicator(),
    &_treeManagementReceiveRequest
  );

  _treeManagementReceiveIsPosted = true;
}

/**
 * Complete asynchronous tree-management payloads without blocking the service callback.
 *
 * Batch responses retain their vectors until MPI has consumed them. A creation becomes visible only after both its
 * worker-ID vector and its master's root state have arrived; one acknowledgement then releases the source rank.
 */
void peano4::parallel::SpacetreeSet::progressPendingTreeManagement() {
  for (auto response = _pendingTreeIdResponses.begin(); response != _pendingTreeIdResponses.end();) {
    int sendCompleted = 0;
    tarch::mpi::test(&response->sendRequest, &sendCompleted, MPI_STATUS_IGNORE);
    response = sendCompleted ? _pendingTreeIdResponses.erase(response) : std::next(response);
  }

  for (auto creation = _pendingTreeCreations.begin(); creation != _pendingTreeCreations.end();) {
    int treeIdsReceived   = 0;
    int rootStateReceived = 0;
    tarch::mpi::test(&creation->treeIdsReceiveRequest, &treeIdsReceived, MPI_STATUS_IGNORE);
    tarch::mpi::test(&creation->rootStateReceiveRequest, &rootStateReceived, MPI_STATUS_IGNORE);

    if (treeIdsReceived and rootStateReceived) {
      for (const int treeId : creation->treeIds) {
        if (Node::getInstance().getRank(treeId) != tarch::mpi::Rank::getInstance().getRank()) {
          tarch::terminateWithError("Received a tree creation request for an ID owned by another rank.");
        }
        _spacetrees.emplace_back(
          treeId,
          creation->request.getMasterSpacetreeId(),
          creation->rootState.x,
          creation->rootState.h,
          creation->rootState.getInverted(),
          _periodicBC
        );
        registerLocalSpacetree(_spacetrees.back());
      }

      sendTreeManagementAcknowledgement(creation->request);
      creation = _pendingTreeCreations.erase(creation);
    } else {
      ++creation;
    }
  }

  for (auto message = _pendingTreeManagementMessages.begin(); message != _pendingTreeManagementMessages.end();) {
    int sendCompleted = 0;
    tarch::mpi::test(&message->sendRequest, &sendCompleted, MPI_STATUS_IGNORE);
    message = sendCompleted ? _pendingTreeManagementMessages.erase(message) : std::next(message);
  }
}

/** Queue an acknowledgement so the service never relies on eager MPI buffering. */
void peano4::parallel::SpacetreeSet::sendTreeManagementAcknowledgement(const TreeManagementMessage& request) {
  auto& pendingMessage   = _pendingTreeManagementMessages.emplace_back();
  pendingMessage.message = TreeManagementMessage(
    request.getMasterSpacetreeId(),
    request.getWorkerSpacetreeId(),
    TreeManagementMessage::Action::Acknowledgement,
    request.getNumberOfSpacetrees()
  );
  tarch::mpi::isend(
    &pendingMessage.message,
    1,
    TreeManagementMessage::getJoinDatatype(),
    request.getSenderRank(),
    getAnswerTag(
      request.getAction() == TreeManagementMessage::Action::CreateNewRemoteTrees ? request.getMasterSpacetreeId()
                                                                                 : request.getWorkerSpacetreeId()
    ),
    tarch::mpi::Rank::getInstance().getCommunicator(),
    &pendingMessage.sendRequest
  );
}
#endif

/** @return Tag used for replies to the target tree. */
int peano4::parallel::SpacetreeSet::getAnswerTag(int targetSpacetreeId) const {
  return _answerMessageTag + Node::getInstance().getLocalTreeId(targetSpacetreeId);
}

/**
 * Process buffered tree-management requests.
 *
 * Requests are buffered because creating or removing a local tree while its
 * collection is being traversed would invalidate iterators. Each request uses
 * variable-length payloads use dedicated tags, which keeps deferred messages
 * from being interpreted as new requests.
 *
 * RequestNewRemoteTrees reserves a complete ID batch and returns it through a
 * non-blocking payload send. CreateNewRemoteTrees posts receives for the ID
 * batch and root state and returns immediately; progressPendingTreeManagement()
 * creates the workers and acknowledges the whole batch once both payloads have
 * arrived. Empty child removal updates the master's child set and queues its
 * acknowledgement.
 *
 * The routine first removes answerable entries from the queue. Message
 * handling can then append new entries without invalidating this pass; a later
 * service poll processes those entries.
 */
void peano4::parallel::SpacetreeSet::answerQuestions() {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1) {
    return;
  }

  // Blocking MPI progress can re-enter this service on the same thread. The
  // guard keeps tree insertion and registration atomic. The service repository
  // excludes concurrent threads, so thread-local state is sufficient.
  static thread_local bool inAnswerQuestions = false;
  if (inAnswerQuestions) {
    return;
  }
  inAnswerQuestions = true;
  class ReentryGuard final {
  public:
    explicit ReentryGuard(bool& flag) :
      _flag(flag) {}
    ReentryGuard(const ReentryGuard&)            = delete;
    ReentryGuard(ReentryGuard&&)                 = delete;
    ReentryGuard& operator=(const ReentryGuard&) = delete;
    ReentryGuard& operator=(ReentryGuard&&)      = delete;
    ~ReentryGuard() { _flag = false; }

  private:
    bool& _flag;
  };
  const ReentryGuard reentryGuard(inAnswerQuestions);
  progressPendingTreeManagement();

  std::vector<peano4::parallel::TreeManagementMessage> answerableMessages;

  auto messageIterator = _unansweredMessages.begin();
  while (messageIterator != _unansweredMessages.end()) {
    switch (messageIterator->getAction()) {
    case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTrees:
      answerableMessages.push_back(*messageIterator);
      messageIterator = _unansweredMessages.erase(messageIterator);
      break;
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty:
    case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTrees:
      answerableMessages.push_back(*messageIterator);
      messageIterator = _unansweredMessages.erase(messageIterator);
      break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      tarch::terminateWithError("Synchronous tree-management messages must not use the asynchronous request tag.");
    }
  }

  for (const auto& message : answerableMessages) {
    switch (message.getAction()) {
    case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTrees: {
      if (message.getNumberOfSpacetrees() < 1 or message.getNumberOfSpacetrees() > MAX_SPACETREES_PER_RANK) {
        tarch::terminateWithError("Received an invalid remote tree reservation batch size.");
      }

      auto& response   = _pendingTreeIdResponses.emplace_back();
      response.treeIds = peano4::parallel::Node::getInstance().reserveIds(
        tarch::mpi::Rank::getInstance().getRank(),
        message.getMasterSpacetreeId(),
        message.getNumberOfSpacetrees()
      );

      tarch::mpi::isend(
        response.treeIds.data(),
        static_cast<int>(response.treeIds.size()),
        MPI_INT,
        message.getSenderRank(),
        _treeIdMessageTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &response.sendRequest
      );
      logDebug("Reserved " << response.treeIds.size() << " tree ids for tree " << message.getMasterSpacetreeId());
    } break;
    case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTrees: {
      if (message.getNumberOfSpacetrees() < 1 or message.getNumberOfSpacetrees() > MAX_SPACETREES_PER_RANK) {
        tarch::terminateWithError("Received an invalid remote tree creation batch size.");
      }

      auto& creation   = _pendingTreeCreations.emplace_back();
      creation.request = message;
      creation.treeIds.resize(message.getNumberOfSpacetrees());
      tarch::mpi::irecv(
        creation.treeIds.data(),
        static_cast<int>(creation.treeIds.size()),
        MPI_INT,
        message.getSenderRank(),
        _treeIdMessageTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &creation.treeIdsReceiveRequest
      );
      tarch::mpi::irecv(
        &creation.rootState,
        1,
        peano4::grid::AutomatonState::getForkDatatype(),
        message.getSenderRank(),
        _stateMessageTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &creation.rootStateReceiveRequest
      );
    } break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      tarch::terminateWithError("Synchronous tree-management messages must not use the asynchronous request tag.");
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty: {
      logInfo(
        "Remote child tree " << message.getWorkerSpacetreeId() << " of local tree " << message.getMasterSpacetreeId()
                             << " became empty and was removed."
      );
      sendTreeManagementAcknowledgement(message);
    } break;
    }
  }
  progressPendingTreeManagement();
#else
  assertion(_unansweredMessages.empty());
#endif
}

/**
 * Poll the posted tree-management receive, buffer a completed request, process
 * the queue, and repost the receive. Buffering keeps receive progress separate
 * from modifications to the local tree collection.
 */
void peano4::parallel::SpacetreeSet::receiveDanglingMessages() {
#ifdef PEANO_WITH_MPI
  if (not _treeManagementReceiveIsPosted) {
    postTreeManagementReceive();
  }

  int        requestCompleted = 0;
  MPI_Status status;

  tarch::mpi::test(&_treeManagementReceiveRequest, &requestCompleted, &status);

  if (requestCompleted) {
    _treeManagementReceiveBuffer.setSenderRank(status.MPI_SOURCE);
    _unansweredMessages.push_back(_treeManagementReceiveBuffer);
    _treeManagementReceiveIsPosted = false;
  }
#endif

  answerQuestions();

#ifdef PEANO_WITH_MPI
  if (not _treeManagementReceiveIsPosted) {
    postTreeManagementReceive();
  }
#endif
}

/**
 * Create worker trees with their master's root state.
 *
 * Workers are grouped by target rank. All remote headers, worker-ID vectors,
 * root states, and acknowledgement receives are posted before the first wait,
 * so independent targets progress concurrently and opposing batches cannot
 * deadlock. One root-state message and one acknowledgement cover every worker
 * for the same master and target rank.
 */
void peano4::parallel::SpacetreeSet::addSpacetrees(const int masterId, const std::vector<int>& newTreeIds) {
  if (newTreeIds.empty()) {
    return;
  }

  const auto& master        = getSpacetree(masterId);
  const int   localRank     = tarch::mpi::Rank::getInstance().getRank();
  const int   numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  std::vector<std::vector<int>> treeIdsByTargetRank(numberOfRanks);
  for (const int treeId : newTreeIds) {
    treeIdsByTargetRank[peano4::parallel::Node::getInstance().getRank(treeId)].push_back(treeId);
  }

  auto& localTreeIds = treeIdsByTargetRank[localRank];
  if (not localTreeIds.empty()) {
    // The workers inherit their master's bounding box, traversal inversion,
    // and periodic configuration; a diverging periodic setup could not detect
    // the same periodic boundaries.
    tarch::multicore::Lock lock(_semaphore);
    for (const int treeId : localTreeIds) {
      _spacetrees
        .emplace_back(treeId, masterId, master._root.x, master._root.h, master._root.getInverted(), master._periodicBC);
      registerLocalSpacetree(_spacetrees.back());
    }
    localTreeIds.clear();
  }

  const auto numberOfRemoteTargets = static_cast<std::size_t>(
    std::count_if(treeIdsByTargetRank.begin(), treeIdsByTargetRank.end(), [](const std::vector<int>& treeIds) {
      return not treeIds.empty();
    })
  );
  if (numberOfRemoteTargets == 0) {
    return;
  }

#ifdef PEANO_WITH_MPI
  struct RemoteCreationBatch final {
    int                   targetRank = -1;
    std::vector<int>      treeIds;
    TreeManagementMessage request;
    TreeManagementMessage acknowledgement;
    MPI_Request           requestSendRequest            = MPI_REQUEST_NULL;
    MPI_Request           treeIdsSendRequest            = MPI_REQUEST_NULL;
    MPI_Request           rootStateSendRequest          = MPI_REQUEST_NULL;
    MPI_Request           acknowledgementReceiveRequest = MPI_REQUEST_NULL;
  };

  const auto                       communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  std::vector<RemoteCreationBatch> batches;
  batches.reserve(numberOfRemoteTargets);
  for (int targetRank = 0; targetRank < numberOfRanks; ++targetRank) {
    auto& treeIds = treeIdsByTargetRank[targetRank];
    if (treeIds.empty()) {
      continue;
    }
    auto& batch      = batches.emplace_back();
    batch.targetRank = targetRank;
    batch.treeIds    = std::move(treeIds);
    batch.request    = TreeManagementMessage(
      masterId,
      -1,
      TreeManagementMessage::Action::CreateNewRemoteTrees,
      static_cast<int>(batch.treeIds.size())
    );

    tarch::mpi::irecv(
      &batch.acknowledgement,
      1,
      TreeManagementMessage::getJoinDatatype(),
      targetRank,
      getAnswerTag(masterId),
      communicator,
      &batch.acknowledgementReceiveRequest
    );
    tarch::mpi::isend(
      &batch.request,
      1,
      TreeManagementMessage::getJoinDatatype(),
      targetRank,
      _requestMessageTag,
      communicator,
      &batch.requestSendRequest
    );
    tarch::mpi::isend(
      batch.treeIds.data(),
      static_cast<int>(batch.treeIds.size()),
      MPI_INT,
      targetRank,
      _treeIdMessageTag,
      communicator,
      &batch.treeIdsSendRequest
    );
    tarch::mpi::isend(
      &master._root,
      1,
      peano4::grid::AutomatonState::getForkDatatype(),
      targetRank,
      _stateMessageTag,
      communicator,
      &batch.rootStateSendRequest
    );
  }

  const auto progress = []() { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };
  for (auto& batch : batches) {
    tarch::mpi::wait(&batch.acknowledgementReceiveRequest, progress, MPI_STATUS_IGNORE);
    assertion(
      batch.acknowledgement.getAction() == TreeManagementMessage::Action::Acknowledgement,
      batch.acknowledgement.toString()
    );
  }
  for (auto& batch : batches) {
    tarch::mpi::wait(&batch.requestSendRequest, progress, MPI_STATUS_IGNORE);
    tarch::mpi::wait(&batch.treeIdsSendRequest, progress, MPI_STATUS_IGNORE);
    tarch::mpi::wait(&batch.rootStateSendRequest, progress, MPI_STATUS_IGNORE);
  }
#else
  assertion(false, "MPI support is disabled.");
#endif
}

peano4::parallel::SpacetreeSet::TraverseTask::TraverseTask(
  grid::Spacetree&             tree,
  grid::GridTraversalObserver& observer,
  bool                         invertTreeTraversalDirectionBeforeWeStart
) :
  Task(Task::NoTaskType, Task::DefaultPriority),
  _spacetree(tree),
  _observer(&observer),
  _invertTreeTraversalDirectionBeforeWeStart(invertTreeTraversalDirectionBeforeWeStart) {}

bool peano4::parallel::SpacetreeSet::TraverseTask::run() {
  if (_invertTreeTraversalDirectionBeforeWeStart) {
    _spacetree._root.setInverted(not _spacetree._root.getInverted());
  }
  assertion(_observer);
  _spacetree.traverse(*_observer);
  return false;
}

/**
 * Create one observer clone for the tree. Repeated calls are idempotent so
 * tree creation during MPI progress can safely prepare a missing clone.
 */
void peano4::parallel::SpacetreeSet::createObserverCloneIfRequired(
  peano4::grid::GridTraversalObserver& observer,
  int                                  treeId
) {
  const auto observerIterator = _clonedObserver.find(treeId);
  if (observerIterator != _clonedObserver.end() and observerIterator->second != nullptr) {
    return;
  }

  auto& observerSlot = _clonedObserver[treeId];
  if (observerSlot == nullptr) {
    observerSlot = observer.clone(treeId);
  }
  assertion(
    _clonedObserver.count(treeId) == 1 and _clonedObserver[treeId] != nullptr,
    treeId,
    _clonedObserver.count(treeId)
  );
}

/**
 * Copy data from splitting trees to their workers.
 *
 * A split spans two sweeps. The first sweep announces the split and clones the
 * grid to the worker. The next sweep streams user data through vertical stacks,
 * lets the worker incorporate it, and restores the traversal order.
 *
 * Spacetree::traverse() inverts the master's traversal direction before this
 * routine runs, so the master's input stack already has the order required by
 * the worker. Both source and destination invoke the exchange to post the MPI
 * send and receive; the local counterpart is empty for a same-rank split.
 * Only vertical stream stacks are transferred. Persistent local stacks remain
 * with their owning tree.
 *
 * Grid-stack clones target disjoint worker stacks and therefore share the
 * available threads. Their task group completes before the ordered MPI and
 * observer streams begin, so message ordering remains unchanged.
 *
 * @see streamDataFromSplittingTreeToNewTree()
 * @see peano4::grid::Spacetree::sendUserData()
 */
void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(
  peano4::grid::GridTraversalObserver& observer
) {
  struct GridCloneJob final {
    const peano4::stacks::Stack<peano4::grid::GridVertex>* source;
    peano4::stacks::Stack<peano4::grid::GridVertex>*       target;
  };

  std::vector<GridCloneJob> gridCloneJobs;
  for (auto& parent : _spacetrees) {
    const int sourceStackNumber = grid::PeanoCurve::getInputStackNumber(parent._root);
    for (auto& worker : parent._hasSplit) {
      const int neighbourSlot = parent.getNeighbourSlot(worker);
      assertion(neighbourSlot >= 0, neighbourSlot, parent._id, worker, parent.toString());

      auto& targetStack = parent.getParallelExchangeVertexStack(
        neighbourSlot,
        grid::StackTopology::ParallelExchangeStack::VerticalOutput
      );
      assertion(targetStack.empty(), parent._id, worker, sourceStackNumber);
      gridCloneJobs.push_back({&parent.getLocalExchangeVertexStack(sourceStackNumber), &targetStack});
    }
  }

  const auto numberOfCloneTasks = std::min(
    gridCloneJobs.size(),
    static_cast<std::size_t>(std::max(1, tarch::multicore::Core::getInstance().getNumberOfThreads()))
  );
  if (numberOfCloneTasks <= 1) {
    for (const auto& cloneJob : gridCloneJobs) {
      cloneJob.target->clone(*cloneJob.source);
    }
  } else {
    std::vector<tarch::multicore::Task*> cloneTasks;
    cloneTasks.reserve(numberOfCloneTasks);
    for (std::size_t taskNumber = 0; taskNumber < numberOfCloneTasks; ++taskNumber) {
      cloneTasks.push_back(new tarch::multicore::TaskWithCopyOfFunctor(
        tarch::multicore::Task::NoTaskType,
        tarch::multicore::Task::DefaultPriority,
        [cloneJobs = &gridCloneJobs, taskNumber, numberOfCloneTasks]() -> bool {
          for (std::size_t jobNumber = taskNumber; jobNumber < cloneJobs->size(); jobNumber += numberOfCloneTasks) {
            const auto& cloneJob = (*cloneJobs)[jobNumber];
            cloneJob.target->clone(*cloneJob.source);
          }
          return false;
        }
      ));
    }
    tarch::multicore::spawnTasks(cloneTasks);
  }

  // Keep stream setup ordered: the MPI metadata handshakes and observer
  // stacks have to match on sender and receiver even though grid cloning does
  // not.
  for (auto& parent : _spacetrees) {
    for (auto& worker : parent._hasSplit) {
      streamDataFromSplittingTreeToNewTree(*this, parent._id, worker);
      // MPI progress can create a remote tree after observer preparation, so
      // the idempotent clone call must precede every dereference here.
      createObserverCloneIfRequired(observer, parent._id);
      _clonedObserver[parent._id]->streamDataFromSplittingTreeToNewTree(worker);
    }
  }

  for (auto& tree : _spacetrees) {
    if (tree._spacetreeState == peano4::grid::SpacetreeState::EmptyRun) {
      streamDataFromSplittingTreeToNewTree(*this, tree._masterId, tree._id);
      createObserverCloneIfRequired(observer, tree._masterId);
      _clonedObserver[tree._masterId]->streamDataFromSplittingTreeToNewTree(tree._id);
    }
  }

  for (auto& tree : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->finishAllOutstandingSendsAndReceives();
  }
}

/**
 * Collect the periodic descriptors recorded by all local trees during the
 * last traversal and resolve them into the routing table. Under MPI a
 * rendezvous completes the picture across ranks: every descriptor travels to
 * a broker rank derived from its key, and each rank receives back the full
 * record set of exactly the keys it participates in. The routing table and
 * the traffic therefore scale with the rank's own periodic surface.
 *
 * The routine reads every tree's recorded descriptors, so it has to run
 * after all local traversals and before any payload routing. With periodic
 * boundary conditions it contains a per-sweep collective, so all ranks reach
 * it in lockstep. On a stationary grid it degenerates to the snapshot
 * comparison plus the one-int allreduce.
 */
void peano4::parallel::SpacetreeSet::rebuildPeriodicAssociations() {
  if (not hasPeriodicBoundaryConditions()) {
    return;
  }

  std::vector<peano4::grid::PeriodicDescriptor> producers;
  std::vector<peano4::grid::PeriodicDescriptor> consumers;
  for (const auto& tree : _spacetrees) {
    producers
      .insert(producers.end(), tree._periodicProducerDescriptors.begin(), tree._periodicProducerDescriptors.end());
    consumers
      .insert(consumers.end(), tree._periodicConsumerDescriptors.begin(), tree._periodicConsumerDescriptors.end());
  }

  const auto descriptorLess = [](
                                const peano4::grid::PeriodicDescriptor& leftDescriptor,
                                const peano4::grid::PeriodicDescriptor& rightDescriptor
                              ) {
    if (leftDescriptor.stamp.key != rightDescriptor.stamp.key) {
      return leftDescriptor.stamp.key < rightDescriptor.stamp.key;
    }
    if (leftDescriptor.stamp.side != rightDescriptor.stamp.side) {
      return static_cast<int>(leftDescriptor.stamp.side) < static_cast<int>(rightDescriptor.stamp.side);
    }
    return leftDescriptor.treeId < rightDescriptor.treeId;
  };
  const auto descriptorEqual = [](
                                 const peano4::grid::PeriodicDescriptor& leftDescriptor,
                                 const peano4::grid::PeriodicDescriptor& rightDescriptor
                               ) {
    return leftDescriptor.stamp.key == rightDescriptor.stamp.key
      and leftDescriptor.stamp.side == rightDescriptor.stamp.side and leftDescriptor.treeId == rightDescriptor.treeId;
  };

  std::sort(producers.begin(), producers.end(), descriptorLess);
  producers.erase(std::unique(producers.begin(), producers.end(), descriptorEqual), producers.end());
  std::sort(consumers.begin(), consumers.end(), descriptorLess);
  consumers.erase(std::unique(consumers.begin(), consumers.end(), descriptorEqual), consumers.end());

  // Sorting keeps the alternating traversal direction from appearing as a
  // topology change.
  const auto descriptorsChanged = [&descriptorEqual](
                                    const std::vector<peano4::grid::PeriodicDescriptor>& currentDescriptors,
                                    const std::vector<peano4::grid::PeriodicDescriptor>& cachedDescriptors
                                  ) {
    return currentDescriptors.size() != cachedDescriptors.size()
      or not std::equal(
             currentDescriptors.begin(),
             currentDescriptors.end(),
             cachedDescriptors.begin(),
             descriptorEqual
      );
  };
  const int localTopologyChanged = descriptorsChanged(producers, _lastPeriodicProducerDescriptors)
      or descriptorsChanged(consumers, _lastPeriodicConsumerDescriptors)
    ? 1
    : 0;

  // A change on one rank can move another rank's partner, so every rank uses
  // the same collective rebuild decision.
  int globalTopologyChanged = localTopologyChanged;
#ifdef PEANO_WITH_MPI
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks > 1) {
    tarch::mpi::allreduce(
      &localTopologyChanged,
      &globalTopologyChanged,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
#endif

  if (not globalTopologyChanged) {
    return;
  }
  _lastPeriodicProducerDescriptors = producers;
  _lastPeriodicConsumerDescriptors = consumers;

#ifdef PEANO_WITH_MPI
  // Rendezvous discovery: every descriptor travels to a broker rank derived
  // from its key, the broker collects both sides of each key, and each
  // participating rank gets back exactly the records of the keys it produces
  // or consumes. Traffic and per-rank table size scale with the local
  // periodic surface. The two alltoall rounds are reached in lockstep by all
  // ranks because globalTopologyChanged is a collective decision, so they
  // cannot deadlock. Tree IDs are global, so getRank()
  // tells each side which rank to route to.
  if (numberOfRanks > 1) {
    constexpr int RecordSize   = peano4::grid::PackedPeriodicDescriptorSize + 1;
    const auto    communicator = tarch::mpi::Rank::getInstance().getCommunicator();

    const auto exchangeBuffers = [&](const std::vector<std::vector<int>>& outgoing) {
      std::vector<int> sendCounts(numberOfRanks, 0);
      std::vector<int> sendDisplacements(numberOfRanks, 0);
      int              totalSendCount = 0;
      for (int rank = 0; rank < numberOfRanks; rank++) {
        sendCounts[rank]        = static_cast<int>(outgoing[rank].size());
        sendDisplacements[rank] = totalSendCount;
        totalSendCount += sendCounts[rank];
      }
      std::vector<int> sendBuffer;
      sendBuffer.reserve(totalSendCount);
      for (const auto& block : outgoing) {
        sendBuffer.insert(sendBuffer.end(), block.begin(), block.end());
      }

      std::vector<int> receiveCounts(numberOfRanks, 0);
      tarch::mpi::alltoall(sendCounts.data(), 1, MPI_INT, receiveCounts.data(), 1, MPI_INT, communicator);

      std::vector<int> receiveDisplacements(numberOfRanks, 0);
      int              totalReceiveCount = 0;
      for (int rank = 0; rank < numberOfRanks; rank++) {
        receiveDisplacements[rank] = totalReceiveCount;
        totalReceiveCount += receiveCounts[rank];
      }
      std::vector<int> receiveBuffer(static_cast<std::size_t>(totalReceiveCount));
      tarch::mpi::alltoallv(
        sendBuffer.data(),
        sendCounts.data(),
        sendDisplacements.data(),
        MPI_INT,
        receiveBuffer.data(),
        receiveCounts.data(),
        receiveDisplacements.data(),
        MPI_INT,
        communicator
      );
      return std::make_pair(std::move(receiveBuffer), std::move(receiveDisplacements));
    };

    std::vector<std::vector<int>> outgoingBrokerRecords(numberOfRanks);
    const auto                    stageDescriptors =
      [&](const std::vector<peano4::grid::PeriodicDescriptor>& descriptors, int descriptorKind) {
        for (const auto& descriptor : descriptors) {
          auto& block = outgoingBrokerRecords[peano4::grid::periodicKeyBroker(descriptor.stamp.key, numberOfRanks)];
          block.resize(block.size() + RecordSize);
          peano4::grid::packPeriodicDescriptor(descriptor, block.data() + block.size() - RecordSize);
          block.back() = descriptorKind;
        }
      };
    stageDescriptors(producers, 0);
    stageDescriptors(consumers, 1);

    auto [brokeredRecords, brokeredDisplacements] = exchangeBuffers(outgoingBrokerRecords);

    // Group the records of each key and remember which ranks reported it;
    // each of those ranks receives the key's complete record set back.
    struct KeyGroup {
      std::vector<std::array<int, RecordSize>> records;
      std::set<int>                            sourceRanks;
    };
    std::map<peano4::grid::PeriodicKey, KeyGroup> groupsByKey;
    for (int sourceRank = 0; sourceRank < numberOfRanks; sourceRank++) {
      const int beginOffset = brokeredDisplacements[sourceRank];
      const int endOffset   = sourceRank + 1 < numberOfRanks ? brokeredDisplacements[sourceRank + 1]
                                                             : static_cast<int>(brokeredRecords.size());
      for (int recordOffset = beginOffset; recordOffset + RecordSize <= endOffset; recordOffset += RecordSize) {
        const auto descriptor = peano4::grid::unpackPeriodicDescriptor(brokeredRecords.data() + recordOffset);
        auto&      group      = groupsByKey[descriptor.stamp.key];
        auto&      record     = group.records.emplace_back();
        std::copy(
          brokeredRecords.begin() + recordOffset,
          brokeredRecords.begin() + recordOffset + RecordSize,
          record.begin()
        );
        group.sourceRanks.insert(sourceRank);
      }
    }

    std::vector<std::vector<int>> replyRecords(numberOfRanks);
    for (const auto& groupEntry : groupsByKey) {
      const auto& group = groupEntry.second;
      for (const int sourceRank : group.sourceRanks) {
        for (const auto& record : group.records) {
          replyRecords[sourceRank].insert(replyRecords[sourceRank].end(), record.begin(), record.end());
        }
      }
    }

    auto receivedReplies = exchangeBuffers(replyRecords).first;

    producers.clear();
    consumers.clear();
    for (std::size_t recordOffset = 0; recordOffset + RecordSize <= receivedReplies.size();
         recordOffset += RecordSize) {
      auto& descriptorList = receivedReplies[recordOffset + RecordSize - 1] == 0 ? producers : consumers;
      descriptorList.push_back(peano4::grid::unpackPeriodicDescriptor(receivedReplies.data() + recordOffset));
    }
    std::sort(producers.begin(), producers.end(), descriptorLess);
    producers.erase(std::unique(producers.begin(), producers.end(), descriptorEqual), producers.end());
    std::sort(consumers.begin(), consumers.end(), descriptorLess);
    consumers.erase(std::unique(consumers.begin(), consumers.end(), descriptorEqual), consumers.end());
  }
#endif

  _periodicRouting = peano4::grid::buildPeriodicRouting(producers, consumers);

  _crossRankPeriodicChannels.clear();
  for (const auto& producer : producers) {
    const auto routingEntry = _periodicRouting.find(producer.stamp.key);
    assertion(routingEntry != _periodicRouting.end(), producer.toString());
    const int producerRank = Node::getInstance().getRank(producer.treeId);
    for (const int targetTreeId :
         routingEntry->second[peano4::grid::toIndex(peano4::grid::opposite(producer.stamp.side))]) {
      if (Node::getInstance().getRank(targetTreeId) != producerRank) {
        _crossRankPeriodicChannels.emplace(producer.treeId, targetTreeId);
      }
    }
  }

  logDebug(
    "Resolved " << producers.size() << " producer / " << consumers.size() << " consumer descriptors into "
                << _periodicRouting.size() << " keys and " << _crossRankPeriodicChannels.size()
                << " cross-rank channels"
  );
}

/**
 * Exchange all boundary data between the trees, after all traversals of a
 * sweep have finished. The routine runs serially and realises four ordered
 * phases per sweep:
 *
 * 1. Resolve the periodic partner associations (rebuildPeriodicAssociations()),
 *    as the routing of every tree depends on them.
 * 2. Per tree, trigger the horizontal sends and receives and purge the
 *    tree's periodic input stacks. Triggering has to precede any wait, since
 *    MPI's rendezvous protocol would otherwise deadlock: a rank must not
 *    block for one tree's data while a peer still waits for it to post
 *    another tree's send.
 * 3. Per tree, route the periodic boundary data. Every purge has to precede
 *    the first routing, because the routing deposits into the partner
 *    trees' input stacks.
 * 4. Wait for all outstanding MPI operations, then run the cross-rank
 *    periodic exchange: once per rank, grid vertices first, then the
 *    user-data containers, in the same order on every rank so the shared
 *    periodic tag stays matched.
 *
 * The data transfer routines modify the underlying stack containers - they
 * create containers and move data around - so the phases must not run
 * concurrently.
 */
void peano4::parallel::SpacetreeSet::exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer) {
  rebuildPeriodicAssociations();

  for (auto& tree : _spacetrees) {
    exchangeAllHorizontalDataExchangeStacks(*this, tree._id);
    purgePeriodicBoundaryDataStacks(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->exchangeAllHorizontalDataExchangeStacks();
    _clonedObserver[tree._id]->purgePeriodicBoundaryDataStacks();
  }

  for (auto& tree : _spacetrees) {
    exchangeAllPeriodicBoundaryDataStacks(*this, tree._id);
    _clonedObserver[tree._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  for (auto& tree : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->finishAllOutstandingSendsAndReceives();
  }

  exchangeCrossRankPeriodicData(*this);
  observer.exchangeCrossRankPeriodicData();
}

/**
 * Snapshot masters that already merge workers before traversal. Traversal can
 * trigger further joins, so callers use this snapshot to distinguish those
 * requests from joins being completed in the current sweep.
 */
std::set<int> peano4::parallel::SpacetreeSet::getLocalTreesMergingWithWorkers() const {
  std::set<int> result;
  for (const auto& tree : _spacetrees) {
    if (not tree._joining.empty()) {
      result.insert(tree._id);
    }
  }
  return result;
}

/**
 * Instantiate workers after local data exchange and tree cleanup. Remote
 * creation may wait for a peer, so starting it while that peer waits for
 * boundary data can deadlock.
 *
 * @see exchangeHorizontalDataBetweenTrees()
 */
void peano4::parallel::SpacetreeSet::createNewTrees() {
  for (const auto& tree : _spacetrees) {
    if (not tree._splitting.empty()) {
      addSpacetrees(tree._id, std::vector<int>(tree._splitting.begin(), tree._splitting.end()));
    }
  }
}

void peano4::parallel::SpacetreeSet::deleteClonedObservers() {
  for (auto& observerEntry : _clonedObserver) {
    if (observerEntry.second != nullptr) {
      delete observerEntry.second;
      observerEntry.second = nullptr;
    }
  }
}

/**
 * Traverse local trees in dependency order.
 *
 * The polling barrier drains deferred tree-management requests on every rank
 * before task lists and communicators are created. Existing and joining trees
 * run first, new workers perform their empty traversal next, and dependent
 * joins finish last. Data exchange, cleanup, and worker creation run only after
 * all traversal tasks complete.
 */
void peano4::parallel::SpacetreeSet::traverse(peano4::grid::GridTraversalObserver& observer) {
#ifdef PEANO_WITH_MPI
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    answerQuestions();
  });
#endif

  Node::getInstance().createPendingCommunicators();

  std::vector<tarch::multicore::Task*> primaryTasks;
  std::vector<tarch::multicore::Task*> secondaryTasks;
  std::vector<tarch::multicore::Task*> tertiaryTasks;

  for (auto& tree : _spacetrees) {
    createObserverCloneIfRequired(observer, tree._id);
    createObserverCloneIfRequired(observer, tree._masterId);
    switch (tree._spacetreeState) {
    case peano4::grid::SpacetreeState::NewRoot:
    case peano4::grid::SpacetreeState::JoinTriggered:
    case peano4::grid::SpacetreeState::Joining:
      primaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    case peano4::grid::SpacetreeState::Running:
      (tree._joining.empty() ? primaryTasks : tertiaryTasks)
        .push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    case peano4::grid::SpacetreeState::EmptyRun:
      secondaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], true));
      tertiaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    default:
      break;
    }
  }

  observer.setLocalTrees();

  tarch::multicore::spawnTasks(primaryTasks);
  streamDataFromSplittingTreesToNewTrees(observer);
  tarch::multicore::spawnTasks(secondaryTasks);
  tarch::multicore::spawnTasks(tertiaryTasks);
  exchangeHorizontalDataBetweenTrees(observer);

  cleanUpTrees(observer);
  createNewTrees();
  deleteClonedObservers();
}

/**
 * Remove joined and empty trees after traversal. Joining workers run before
 * their masters so vertical buffers contain the data when the master consumes
 * them; the traversal phases therefore keep a worker and its master out of the
 * same parallel task group.
 */
void peano4::parallel::SpacetreeSet::cleanUpTrees(peano4::grid::GridTraversalObserver& observer) {
  for (auto treeIterator = _spacetrees.begin(); treeIterator != _spacetrees.end();) {
    if (treeIterator->_spacetreeState == peano4::grid::SpacetreeState::Joined) {
      logInfo("Tree " << treeIterator->_id << " has successfully joined");
      const int treeId = treeIterator->_id;
      unregisterLocalSpacetree(treeId);
      Node::getInstance().deregisterId(treeIterator->_id);
      treeIterator = _spacetrees.erase(treeIterator);
      continue;
    } else if (treeIterator->mayJoinWithMaster()
               and treeIterator->getGridStatistics().numberOfLocalUnrefinedCells == 0) {
      logInfo("Remove empty tree " << treeIterator->_id << " with master " << treeIterator->_masterId);
      const int treeId = treeIterator->_id;
      unregisterLocalSpacetree(treeId);
      Node::getInstance().deregisterId(treeIterator->_id);

      if (Node::getInstance().getRank(treeIterator->_masterId) != tarch::mpi::Rank::getInstance().getRank()) {
        logDebug("Parent tree " << treeIterator->_masterId << " is not local on this rank. Remove child reference");
#ifdef PEANO_WITH_MPI
        TreeManagementMessage message(
          treeIterator->_masterId,
          treeIterator->_id,
          TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty
        );
        TreeManagementMessage::send(
          message,
          Node::getInstance().getRank(treeIterator->_masterId),
          _requestMessageTag,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );

        TreeManagementMessage::receive(
          message,
          Node::getInstance().getRank(treeIterator->_masterId),
          getAnswerTag(treeIterator->_id),
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
        assertion(false, "This branch requires MPI support.");
#endif
      } else {
        logDebug("Parent tree " << treeIterator->_masterId << " is local on this rank");
        getSpacetree(treeIterator->_masterId)._statistics.setRemovedEmptySubtree(true);
      }
      treeIterator = _spacetrees.erase(treeIterator);
      continue;
    } else if (treeIterator->mayJoinWithMaster()) {
      const int localRank  = Node::getInstance().getRank(treeIterator->_id);
      const int masterRank = Node::getInstance().getRank(treeIterator->_masterId);
      if (localRank == masterRank
          and getSpacetree(treeIterator->_masterId).getGridStatistics().getCoarseningHasBeenVetoed()) {
        logError(
          "Join tree " << treeIterator->_id << " as it is deteriorated (encodes no hierarchical data) while master "
                       << treeIterator->_masterId << " resides on same rank and can't coarsen"
        );
        logError("Not implemented yet");
      } else if (localRank == masterRank) {
        logDebug(
          "Tree " << treeIterator->_id
                  << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master"
        );
      } else {
        logDebug(
          "Tree " << treeIterator->_id << " should join to reduce synchronisation: " << treeIterator->toString()
        );
        logDebug("Not implemented yet");
      }
    }
    ++treeIterator;
  }
}

const peano4::grid::GridStatistics& peano4::parallel::SpacetreeSet::getGridStatistics(int treeId) const {
  assertion(isLocalSpacetree(treeId), treeId, tarch::mpi::Rank::getInstance().getRank());
  return getSpacetree(treeId).getGridStatistics();
}

peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  grid::GridStatistics result;
  bool                 hasStatistics = false;

  for (const auto& tree : _spacetrees) {
    if (not hasStatistics) {
      result        = tree._statistics;
      hasStatistics = true;
    } else {
      result += tree._statistics;
    }
  }

  return hasStatistics ? result : grid::GridStatistics{};
}

bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  if (localTreeId < 0 or localTreeId >= MAX_SPACETREES_PER_RANK) {
    return false;
  }
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  return tree != nullptr and tree->_id == treeId;
}

/**
 * @return Set of local tree IDs.
 */
std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;
  for (const auto& tree : _spacetrees) {
    result.insert(tree._id);
  }
  return result;
}

/** Report whether a local tree can register another split. */
bool peano4::parallel::SpacetreeSet::maySplit(const int treeId) const { return getSpacetree(treeId).maySplit(); }

/**
 * Register an exact split on a local tree.
 *
 * Peano rejects the request and returns false if maySplit() on the underlying
 * tree returns false or no tree ID is available. A local target rank creates a
 * worker for shared-memory traversal; a remote target uses the tree-management
 * handshake.
 *
 * @param treeId Tree that gives cells to the worker.
 * @param instruction Split mode and exact number of current leaf cells to transfer.
 * @param targetRank Rank that hosts the worker.
 *
 * @return Whether the exact split request was registered successfully. Once
 *   registered, failure to assign the requested cells terminates the run.
 */
bool peano4::parallel::SpacetreeSet::split(
  const int                     treeId,
  const grid::SplitInstruction& instruction,
  const int                     targetRank
) {
  return split(treeId, std::vector<SplitRequest>{{instruction, targetRank}});
}

/**
 * Reserve worker IDs and register a batch of exact splits.
 *
 * Requests are grouped by target rank only for allocation. The resulting IDs
 * are mapped back to their original request positions before the source tree
 * sees any instruction, preserving deterministic split order. Every remote
 * receive and request send is posted before the first wait, so latency grows
 * with the slowest target rather than the sum of all per-tree round trips.
 */
bool peano4::parallel::SpacetreeSet::split(const int treeId, const std::vector<SplitRequest>& requests) {
  if (requests.empty()) {
    return true;
  }

  auto& tree = getSpacetree(treeId);
  if (not tree.maySplit()) {
    return false;
  }

  const int localRank     = tarch::mpi::Rank::getInstance().getRank();
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  std::vector<std::vector<std::size_t>> requestIndicesByTargetRank(numberOfRanks);
  for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex) {
    const int targetRank = requests[requestIndex].targetRank;
    if (targetRank < 0 or targetRank >= numberOfRanks) {
      tarch::terminateWithError("Cannot split a tree onto an invalid target rank.");
    }
    requestIndicesByTargetRank[targetRank].push_back(requestIndex);
  }

  std::vector<int> workerTreeIds(requests.size(), -1);
  auto&            localRequests = requestIndicesByTargetRank[localRank];
  if (not localRequests.empty()) {
    const auto localTreeIds =
      peano4::parallel::Node::getInstance().reserveIds(localRank, treeId, static_cast<int>(localRequests.size()));
    for (std::size_t tree = 0; tree < localTreeIds.size(); ++tree) {
      workerTreeIds[localRequests[tree]] = localTreeIds[tree];
    }
    localRequests.clear();
  }

  const auto numberOfRemoteTargets = static_cast<std::size_t>(std::count_if(
    requestIndicesByTargetRank.begin(),
    requestIndicesByTargetRank.end(),
    [](const std::vector<std::size_t>& requestIndices) { return not requestIndices.empty(); }
  ));

#ifdef PEANO_WITH_MPI
  struct RemoteReservationBatch final {
    int                      targetRank = -1;
    std::vector<std::size_t> requestIndices;
    std::vector<int>         treeIds;
    TreeManagementMessage    request;
    MPI_Request              requestSendRequest    = MPI_REQUEST_NULL;
    MPI_Request              treeIdsReceiveRequest = MPI_REQUEST_NULL;
  };

  const auto                          communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  std::vector<RemoteReservationBatch> batches;
  batches.reserve(numberOfRemoteTargets);
  for (int targetRank = 0; targetRank < numberOfRanks; ++targetRank) {
    auto& requestIndices = requestIndicesByTargetRank[targetRank];
    if (requestIndices.empty()) {
      continue;
    }
    auto& batch          = batches.emplace_back();
    batch.targetRank     = targetRank;
    batch.requestIndices = std::move(requestIndices);
    batch.treeIds.resize(batch.requestIndices.size());
    batch.request = TreeManagementMessage(
      treeId,
      -1,
      TreeManagementMessage::Action::RequestNewRemoteTrees,
      static_cast<int>(batch.treeIds.size())
    );

    tarch::mpi::irecv(
      batch.treeIds.data(),
      static_cast<int>(batch.treeIds.size()),
      MPI_INT,
      targetRank,
      _treeIdMessageTag,
      communicator,
      &batch.treeIdsReceiveRequest
    );
    tarch::mpi::isend(
      &batch.request,
      1,
      TreeManagementMessage::getJoinDatatype(),
      targetRank,
      _requestMessageTag,
      communicator,
      &batch.requestSendRequest
    );
  }

  const auto progress = []() { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };
  for (auto& batch : batches) {
    tarch::mpi::wait(&batch.treeIdsReceiveRequest, progress, MPI_STATUS_IGNORE);
  }
  for (auto& batch : batches) {
    tarch::mpi::wait(&batch.requestSendRequest, progress, MPI_STATUS_IGNORE);
    for (std::size_t tree = 0; tree < batch.treeIds.size(); ++tree) {
      const int treeIdOnTarget = batch.treeIds[tree];
      if (treeIdOnTarget < 0 or peano4::parallel::Node::getInstance().getRank(treeIdOnTarget) != batch.targetRank) {
        tarch::terminateWithError("Received an invalid worker tree ID for a remote split.");
      }
      workerTreeIds[batch.requestIndices[tree]] = treeIdOnTarget;
    }
  }
#else
  if (numberOfRemoteTargets > 0) {
    assertion(false, "A remote split requires MPI support.");
  }
#endif

  for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex) {
    const int workerTreeId = workerTreeIds[requestIndex];
    if (workerTreeId < 0) {
      tarch::terminateWithError("Could not reserve a worker tree ID for an exact split.");
    }
    tree.split(workerTreeId, requests[requestIndex].instruction);
    logDebug(
      "Trigger split of tree " << treeId << " into tree " << workerTreeId << " on rank "
                               << requests[requestIndex].targetRank << " with instruction "
                               << requests[requestIndex].instruction
    );
  }

  return true;
}

peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int treeId) {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    treeId,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found.", treeId, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == treeId, "Spacetree lookup mismatch.", treeId, tree->_id, localTreeId);
  return *tree;
}

const peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int treeId) const {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    treeId,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found.", treeId, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == treeId, "Spacetree lookup mismatch.", treeId, tree->_id, localTreeId);
  return *tree;
}
