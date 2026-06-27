// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet.h"

#include "peano4/parallel/Node.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/StackTopology.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/MemoryUsage.h"

peano4::parallel::SpacetreeSet peano4::parallel::SpacetreeSet::_singleton;

tarch::multicore::BooleanSemaphore peano4::parallel::SpacetreeSet::_semaphore;

peano4::parallel::SpacetreeSet& peano4::parallel::SpacetreeSet::getInstance() { return _singleton; }

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::parallel::SpacetreeSet::getLocalExchangeStack(
  int treeId,
  int localStackId
) {
  return getSpacetree(treeId).getLocalExchangeVertexStack(localStackId);
}

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::parallel::SpacetreeSet::
  getParallelExchangeStack(int treeId, int slot, peano4::grid::StackTopology::ParallelExchangeStack which) {
  return getSpacetree(treeId).getParallelExchangeVertexStack(slot, which);
}

void peano4::parallel::SpacetreeSet::registerLocalSpacetree(peano4::grid::Spacetree& tree) {
  const int localTreeId = Node::getInstance().getLocalTreeId(tree._id);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  assertion(
    _spacetreeByLocalTreeId[localTreeId] == nullptr,
    "Duplicate local tree id",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  _spacetreeByLocalTreeId[localTreeId] = &tree;
}

void peano4::parallel::SpacetreeSet::unregisterLocalSpacetree(int id) {
  const int localTreeId = Node::getInstance().getLocalTreeId(id);
  if (localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK) {
    [[maybe_unused]] auto* const tree = _spacetreeByLocalTreeId[localTreeId];
    assertion(
      tree == nullptr or tree->_id == id,
      "Spacetree lookup mismatch",
      id,
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
  _answerMessageTag  = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::SpacetreeSet - answer message",
    MAX_SPACETREES_PER_RANK
  );
  _stateMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - automaton state message");
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

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _spacetrees.emplace_back(offset, width, periodicBC);
    registerLocalSpacetree(_spacetrees.back());
  }
}

void peano4::parallel::SpacetreeSet::shutdown() {
  tarch::services::ServiceRepository::getInstance().removeService(this);

#ifdef PEANO_WITH_MPI
  if (_treeManagementReceiveIsPosted) {
    tarch::mpi::cancel(&_treeManagementReceiveRequest);
    tarch::mpi::wait(&_treeManagementReceiveRequest, []() {}, MPI_STATUS_IGNORE);
    _treeManagementReceiveRequest  = MPI_REQUEST_NULL;
    _treeManagementReceiveIsPosted = false;
  }
#endif
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
#endif

int peano4::parallel::SpacetreeSet::getAnswerTag(int targetSpacetreeId) const {
  return _answerMessageTag + Node::getInstance().getLocalTreeId(targetSpacetreeId);
}

void peano4::parallel::SpacetreeSet::answerQuestions() {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1) {
    return;
  }

  // The handshake below issues blocking MPI that pumps the service repository, re-entering this
  // routine on the same thread. A nested answerQuestions would slip between _spacetrees.emplace_back
  // and _spacetrees.back(), registering the wrong tree (the intermittent rebalance segfault). Skip
  // re-entrant calls so tree creation stays atomic and lock-free.
  // Concurrent entry from another thread is already excluded by the service repository's
  // recursive try-lock, so a thread-local flag suffices.
  static thread_local bool inAnswerQuestions = false;
  if (inAnswerQuestions) {
    return;
  }
  inAnswerQuestions = true;
  struct ReentryGuard {
    bool& flag;
    ~ReentryGuard() { flag = false; }
  } reentryGuard{inAnswerQuestions};

  std::vector<peano4::parallel::TreeManagementMessage> unansweredMessagesThatIanAnswerNow;

  std::vector<peano4::parallel::TreeManagementMessage>::iterator it = _unansweredMessages.begin();
  while (it != _unansweredMessages.end()) {
    switch (it->getAction()) {
    case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree:
      unansweredMessagesThatIanAnswerNow.push_back(*it);
      it = _unansweredMessages.erase(it);
      break;
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty:
    case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTree:
      unansweredMessagesThatIanAnswerNow.push_back(*it);
      it = _unansweredMessages.erase(it);
      break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      assertion(false, "Should only be passed synchronously and never run through this tag");
      break;
    }
  }

  for (const auto& p : unansweredMessagesThatIanAnswerNow) {
    switch (p.getAction()) {
    case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree: {
      int newSpacetreeId = peano4::parallel::Node::getInstance().reserveId(
        tarch::mpi::Rank::getInstance().getRank(), // on current node
        p.getMasterSpacetreeId()                   // this is the tree who has requested the new tree
      );

      peano4::parallel::TreeManagementMessage answerMessage;
      answerMessage.setWorkerSpacetreeId(newSpacetreeId);
      answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
      peano4::parallel::TreeManagementMessage::send(
        answerMessage,
        p.getSenderRank(),
        getAnswerTag(p.getMasterSpacetreeId()),
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      logDebug("Reserved tree id " << newSpacetreeId << " for tree " << p.getMasterSpacetreeId());
    } break;
    case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTree: {
      peano4::parallel::TreeManagementMessage answerMessage;
      answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
      peano4::parallel::TreeManagementMessage::send(
        answerMessage,
        p.getSenderRank(),
        getAnswerTag(p.getMasterSpacetreeId()),
        tarch::mpi::Rank::getInstance().getCommunicator()
      );

      peano4::grid::AutomatonState state;
      peano4::grid::AutomatonState::
        receive(state, p.getSenderRank(), _stateMessageTag, tarch::mpi::Rank::getInstance().getCommunicator());

      _spacetrees
        .emplace_back(p.getWorkerSpacetreeId(), p.getMasterSpacetreeId(), state.x, state.h, state.getInverted());
      registerLocalSpacetree(_spacetrees.back());

      peano4::parallel::TreeManagementMessage::send(
        answerMessage,
        p.getSenderRank(),
        getAnswerTag(p.getMasterSpacetreeId()),
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
    } break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      assertion(false, "Should only be passed synchronously and never run through this tag");
      break;
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty: {
      logInfo(
        "Learned that remote child tree "
        << p.getWorkerSpacetreeId() << " of local tree " << p.getMasterSpacetreeId()
        << " is degenerated thus had been removed"
      );
      getSpacetree(p.getMasterSpacetreeId())._childrenIds.erase(p.getWorkerSpacetreeId());

      peano4::parallel::TreeManagementMessage answerMessage;
      answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
      peano4::parallel::TreeManagementMessage::send(
        answerMessage,
        p.getSenderRank(),
        getAnswerTag(p.getWorkerSpacetreeId()),
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
    } break;
    }
  }
#else
  assertion(_unansweredMessages.empty());
#endif
}

void peano4::parallel::SpacetreeSet::receiveDanglingMessages() {
#ifdef PEANO_WITH_MPI
  if (not _treeManagementReceiveIsPosted) {
    postTreeManagementReceive();
  }

  int        completed = 0;
  MPI_Status status;

  tarch::mpi::test(&_treeManagementReceiveRequest, &completed, &status);

  if (completed) {
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

void peano4::parallel::SpacetreeSet::addSpacetree(int masterId, int newTreeId) {
  if (peano4::parallel::Node::getInstance().getRank(masterId)
      != peano4::parallel::Node::getInstance().getRank(newTreeId)) {
#ifdef PEANO_WITH_MPI
    const int targetRank = peano4::parallel::Node::getInstance().getRank(newTreeId);

    TreeManagementMessage message;
    message.setMasterSpacetreeId(masterId);
    message.setWorkerSpacetreeId(newTreeId);
    message.setAction(TreeManagementMessage::Action::CreateNewRemoteTree);
    TreeManagementMessage::sendAndPollDanglingMessages(message, targetRank, _requestMessageTag);

    TreeManagementMessage::
      receive(message, targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator());
    assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());

    const peano4::grid::AutomatonState& state = _spacetrees.begin()->_root;
    logDebug("Send state " << state.toString() << " to rank " << targetRank);
    peano4::grid::AutomatonState::
      send(state, targetRank, _stateMessageTag, tarch::mpi::Rank::getInstance().getCommunicator());

    TreeManagementMessage::
      receive(message, targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator());
    assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
    assertion(false, "MPI build disabled");
#endif
  } else {
    tarch::multicore::Lock lock(_semaphore);
    _spacetrees.emplace_back(
      newTreeId,
      masterId,
      _spacetrees.begin()->_root.x,
      _spacetrees.begin()->_root.h,
      _spacetrees.begin()->_root.getInverted()
    );
    registerLocalSpacetree(_spacetrees.back());
  }
}

peano4::parallel::SpacetreeSet::TraverseTask::TraverseTask(
  grid::Spacetree&             tree,
  grid::GridTraversalObserver& observer,
  bool                         invertTreeTraversalDirectionBeforeWeStart
):
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

void peano4::parallel::SpacetreeSet::createObserverCloneIfRequired(
  peano4::grid::GridTraversalObserver& observer,
  int                                  treeId
) {
  auto it = _clonedObserver.find(treeId);
  if (it != _clonedObserver.end() and it->second != nullptr) {
    return;
  }

  auto& slot = _clonedObserver[treeId];
  if (slot == nullptr) {
    slot = observer.clone(treeId);
  }
  assertion(
    _clonedObserver.count(treeId) == 1 and _clonedObserver[treeId] != nullptr,
    treeId,
    _clonedObserver.count(treeId)
  );
}

void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(
  peano4::grid::GridTraversalObserver& observer
) {
  for (auto& parent : _spacetrees) {
    const int sourceStackNumber = grid::PeanoCurve::getInputStackNumber(parent._root);
    for (auto& worker : parent._hasSplit) {
      const int slot = parent.getNeighbourSlot(worker);
      assertion(slot >= 0, slot, parent._id, worker, parent.toString());

      auto& targetStack = parent.getParallelExchangeVertexStack(
        slot,
        grid::StackTopology::ParallelExchangeStack::VerticalOutput
      );
      assertion(targetStack.empty(), parent._id, worker, sourceStackNumber);
      targetStack.clone(parent.getLocalExchangeVertexStack(sourceStackNumber));

      streamDataFromSplittingTreeToNewTree(*this, parent._id, worker);
      // A remote tree can be created mid-exchange (answerQuestions runs from receiveDanglingMessages
      // during MPI progress), after the pre-traversal clone pass. Clone its observer just in time -
      // the call is idempotent for already-cloned trees - so it is never a null dereference.
      createObserverCloneIfRequired(observer, parent._id);
      _clonedObserver[parent._id]->streamDataFromSplittingTreeToNewTree(worker);
    }
  }

  for (auto& p : _spacetrees) {
    if (p._spacetreeState == peano4::grid::SpacetreeState::EmptyRun) {
      streamDataFromSplittingTreeToNewTree(*this, p._masterId, p._id);
      createObserverCloneIfRequired(observer, p._masterId);
      _clonedObserver[p._masterId]->streamDataFromSplittingTreeToNewTree(p._id);
    }
  }

  for (auto& p : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, p._id);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }
}

void peano4::parallel::SpacetreeSet::exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer) {
  for (auto& p : _spacetrees) {
    exchangeAllHorizontalDataExchangeStacks(*this, p._id);
    exchangeAllPeriodicBoundaryDataStacks(*this, p._id);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->exchangeAllHorizontalDataExchangeStacks();
    _clonedObserver[p._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  for (auto& p : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, p._id);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }
}

std::set<int> peano4::parallel::SpacetreeSet::getLocalTreesMergingWithWorkers() const {
  std::set<int> result;
  for (const auto& tree : _spacetrees) {
    if (not tree._joining.empty()) {
      result.insert(tree._id);
    }
  }
  return result;
}

void peano4::parallel::SpacetreeSet::createNewTrees() {
  for (const auto& tree : _spacetrees) {
    for (auto& p : tree._splitting) {
      addSpacetree(tree._id, p);
    }
  }
}

void peano4::parallel::SpacetreeSet::deleteClonedObservers() {
  for (auto& p : _clonedObserver) {
    if (p.second != nullptr) {
      delete p.second;
      p.second = nullptr;
    }
  }
}

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

  for (auto& p : _spacetrees) {
    createObserverCloneIfRequired(observer, p._id);
    createObserverCloneIfRequired(observer, p._masterId);
    switch (p._spacetreeState) {
    case peano4::grid::SpacetreeState::NewRoot:
    case peano4::grid::SpacetreeState::JoinTriggered:
    case peano4::grid::SpacetreeState::Joining:
      primaryTasks.push_back(new TraverseTask(p, *_clonedObserver[p._id], false));
      break;
    case peano4::grid::SpacetreeState::Running:
      (p._joining.empty() ? primaryTasks : tertiaryTasks)
        .push_back(new TraverseTask(p, *_clonedObserver[p._id], false));
      break;
    case peano4::grid::SpacetreeState::EmptyRun:
      secondaryTasks.push_back(new TraverseTask(p, *_clonedObserver[p._id], true));
      tertiaryTasks.push_back(new TraverseTask(p, *_clonedObserver[p._id], false));
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

void peano4::parallel::SpacetreeSet::cleanUpTrees(peano4::grid::GridTraversalObserver& observer) {
  for (auto p = _spacetrees.begin(); p != _spacetrees.end();) {
    /*
        else if (
          p->getGridStatistics().getCoarseningHasBeenVetoed()
          and
          p->mayJoinWithMaster()
          and
        ) {
          logInfo("trigger join of tree " << p->_id << " with its master tree " << p->_masterId
     << " to enable further grid erases"); join(p->_id);
     //
          p->_masterId>=0
          and
          getSpacetree(p->_masterId).mayJoinWithWorker()
        }
    */
    if (p->_spacetreeState == peano4::grid::SpacetreeState::Joined) {
      logInfo("Tree " << p->_id << " has successfully joined");
      const int id = p->_id;
      unregisterLocalSpacetree(id);
      Node::getInstance().deregisterId(p->_id);
      p = _spacetrees.erase(p);
    } else if (p->mayJoinWithMaster() and p->getGridStatistics().numberOfLocalUnrefinedCells == 0) {
      logInfo("Remove empty tree " << p->_id << " with master " << p->_masterId);
      const int id = p->_id;
      unregisterLocalSpacetree(id);
      Node::getInstance().deregisterId(p->_id);

      if (Node::getInstance().getRank(p->_masterId) != tarch::mpi::Rank::getInstance().getRank()) {
        logDebug("Parent tree " << p->_masterId << " is not local on this rank. Remove child reference");
#ifdef PEANO_WITH_MPI
        TreeManagementMessage
          message(p->_masterId, p->_id, TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty);
        TreeManagementMessage::send(
          message,
          Node::getInstance().getRank(p->_masterId),
          _requestMessageTag,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );

        TreeManagementMessage::receive(
          message,
          Node::getInstance().getRank(p->_masterId),
          getAnswerTag(p->_id),
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
        assertion(false, "Branch may not be entered");
#endif
      } else {
        logDebug("Parent tree " << p->_masterId << " is local on this rank. Remove child reference");
        getSpacetree(p->_masterId)._childrenIds.erase(p->_id);
        getSpacetree(p->_masterId)._statistics.setRemovedEmptySubtree(true);
      }
      p = _spacetrees.erase(p);
    } else if (p->mayJoinWithMaster()) {
      const int localRank  = Node::getInstance().getRank(p->_id);
      const int masterRank = Node::getInstance().getRank(p->_masterId);
      if (localRank == masterRank and getSpacetree(p->_masterId).getGridStatistics().getCoarseningHasBeenVetoed()) {
        // @todo erste Meldung info
        logError(
          "Join tree "
          << p->_id << " as it is deteriorated (encodes no hierarchical data) while master " << p->_masterId
          << " resides on same rank and can't coarsen"
        );
        logError("Not implemented yet");
        // p->joinWithMaster();
        // getSpacetree(p->_masterId).joinWithWorker(p->_id);
      } else if (localRank == masterRank) {
        logDebug(
          "Tree " << p->_id << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master"
        );
      } else {
        // @todo: Aber nur, wenn es noch andere Baeume auf diesem Rank gibt
        // @todo erste Meldung info
        logDebug("I should merge tree " << p->_id << " to reduce synchronisation: " << p->toString());
        logDebug("Not implemented yet");
      }
    }
    p++;
  }
}

const peano4::grid::GridStatistics& peano4::parallel::SpacetreeSet::getGridStatistics(int treeId) const {
  assertion(isLocalSpacetree(treeId), treeId, tarch::mpi::Rank::getInstance().getRank());
  return getSpacetree(treeId).getGridStatistics();
}

peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  grid::GridStatistics result;
  bool                 any = false;

  for (const auto& tree : _spacetrees) {
    if (tree._spacetreeState == grid::SpacetreeState::NewFromSplit) {
      continue;
    }
    if (!any) {
      result = tree._statistics;
      any    = true;
    } else {
      result += tree._statistics;
    }
  }

  return any ? result : grid::GridStatistics{};
}

bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  if (localTreeId < 0 or localTreeId >= MAX_SPACETREES_PER_RANK) {
    return false;
  }
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  return tree != nullptr and tree->_id == treeId;
}

std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;
  for (auto& p : _spacetrees) {
    result.insert(p._id);
  }
  return result;
}

bool peano4::parallel::SpacetreeSet::split(int treeId, const grid::SplitInstruction& instruction, int targetRank) {
  peano4::grid::Spacetree& tree = getSpacetree(treeId);

  if (tree.maySplit()) {
    int newSpacetreeId = -1;

    if (tarch::mpi::Rank::getInstance().getRank() != targetRank) {
#ifdef PEANO_WITH_MPI
      logDebug("Request new tree on rank " << targetRank);
      peano4::parallel::TreeManagementMessage requestMessage;
      requestMessage.setMasterSpacetreeId(treeId);
      requestMessage.setWorkerSpacetreeId(-1);
      requestMessage.setAction(peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree);
      TreeManagementMessage::sendAndPollDanglingMessages(requestMessage, targetRank, _requestMessageTag);

      logDebug("Message " << requestMessage.toString() << " sent - wait for answer");

      peano4::parallel::TreeManagementMessage answerMessage;
      TreeManagementMessage::receiveAndPollDanglingMessages(answerMessage, targetRank, getAnswerTag(treeId));
      assertion(answerMessage.getAction() == TreeManagementMessage::Action::Acknowledgement);
      newSpacetreeId = answerMessage.getWorkerSpacetreeId();
#else
      newSpacetreeId = -1;
      assertion(false, "Can't split into tree on a different rank if not compiled with parallel support");
#endif
    } else {
      newSpacetreeId = peano4::parallel::Node::getInstance().reserveId(
        peano4::parallel::Node::getInstance().getRank(treeId),
        treeId
      );
    }

    if (newSpacetreeId >= 0) {
      tree.split(newSpacetreeId, instruction);
      logDebug(
        "Trigger split of tree "
        << treeId << " into tree " << newSpacetreeId << " on rank " << targetRank << " with instruction " << instruction
      );
      return true;
    }
  }

  return false;
}

peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int id) {
  const int localTreeId = Node::getInstance().getLocalTreeId(id);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id",
    id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found", id, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == id, "Spacetree lookup mismatch", id, tree->_id, localTreeId);
  return *tree;
}

const peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int id) const {
  const int localTreeId = Node::getInstance().getLocalTreeId(id);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id",
    id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  const peano4::grid::Spacetree* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found", id, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == id, "Spacetree lookup mismatch", id, tree->_id, localTreeId);
  return *tree;
}
