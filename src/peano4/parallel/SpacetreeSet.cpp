// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet.h"

#include "Node.h"
#include "tarch/tarch.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/timing/Watch.h"

tarch::logging::Log peano4::parallel::SpacetreeSet::_log("peano4::parallel::SpacetreeSet");

peano4::parallel::SpacetreeSet peano4::parallel::SpacetreeSet::_singleton;

tarch::multicore::BooleanSemaphore peano4::parallel::SpacetreeSet::_semaphore;

peano4::parallel::SpacetreeSet& peano4::parallel::SpacetreeSet::getInstance() { return _singleton; }

void peano4::parallel::SpacetreeSet::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  _requestMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - request message");
  _answerMessageTag  = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - answer message", Node::MaxSpacetreesPerRank);
  tarch::services::ServiceRepository::getInstance().addService(this, "peano4::parallel::SpacetreeSet");

#ifdef WITH_MPI
  assertion4(
    (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees() == 1
     and tarch::mpi::Rank::getInstance().getRank() == 0
    ) or (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees() == 0 and tarch::mpi::Rank::getInstance().getRank() != 0),
    peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees(),
    offset,
    width,
    tarch::mpi::Rank::getInstance().getRank()
  );
#endif

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    peano4::grid::Spacetree spacetree(offset, width, periodicBC);
    _spacetrees.push_back(spacetree);
  }
}

void peano4::parallel::SpacetreeSet::shutdown() {
  peano4::grid::Spacetree::_vertexStack.clear();
  tarch::services::ServiceRepository::getInstance().removeService(this);
}

int peano4::parallel::SpacetreeSet::getAnswerTag(int targetSpacetreeId) const {
  return _answerMessageTag + Node::getInstance().getLocalTreeId(targetSpacetreeId);
}

void peano4::parallel::SpacetreeSet::answerQuestions() {
#ifdef WITH_MPI
  std::vector<peano4::parallel::TreeManagementMessage> unansweredMessagesThatIanAnswerNow;

  std::vector<peano4::parallel::TreeManagementMessage>::iterator p = _unansweredMessages.begin();
  while (p != _unansweredMessages.end()) {
    switch (p->getAction()) {
    case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree:
      unansweredMessagesThatIanAnswerNow.push_back(*p);
      p = _unansweredMessages.erase(p);
      break;
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty:
    case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTree:
      unansweredMessagesThatIanAnswerNow.push_back(*p);
      p = _unansweredMessages.erase(p);
      break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      assertionMsg(false, "Should only be passed synchronously and never run through this tag");
      break;
    }
  }

  for (auto p : unansweredMessagesThatIanAnswerNow) {
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
      logDebug(
        "receiveDanglingMessages()", "Reserved tree id " << newSpacetreeId << " for tree " << p.getMasterSpacetreeId()
      );
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
      peano4::grid::AutomatonState::receive(
        state, p.getSenderRank(), _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator()
      );
      peano4::grid::Spacetree newTree(
        p.getWorkerSpacetreeId(), p.getMasterSpacetreeId(), state.x, state.h, state.getInverted()
      );

      _spacetrees.push_back(std::move(newTree));

      peano4::parallel::TreeManagementMessage::send(
        answerMessage,
        p.getSenderRank(),
        getAnswerTag(p.getMasterSpacetreeId()),
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
    } break;
    case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
    case peano4::parallel::TreeManagementMessage::Action::JoinWithWorker:
      assertionMsg(false, "Should only be passed synchronously and never run through this tag");
      break;
    case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty: {
      logInfo(
        "receiveDanglingMessages()",
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
#ifdef WITH_MPI
  if (tarch::mpi::Rank::getInstance().isMessageInQueue(_requestMessageTag)) {
    peano4::parallel::TreeManagementMessage message;
    peano4::parallel::TreeManagementMessage::receive(
      message, MPI_ANY_SOURCE, _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator()
    );

    _unansweredMessages.push_back(message);
    logDebug("receiveDanglingMessages()", "Received new message " << message.toString());
  }

  answerQuestions();
#endif
}


void peano4::parallel::SpacetreeSet::addSpacetree(int masterId, int newTreeId) {
  logTraceInWith2Arguments("addSpacetree()", masterId, newTreeId);
  if (peano4::parallel::Node::getInstance().getRank(masterId) != peano4::parallel::Node::getInstance().getRank(newTreeId)) {
#ifdef WITH_MPI
    const int targetRank = peano4::parallel::Node::getInstance().getRank(newTreeId);

    TreeManagementMessage message;
    message.setMasterSpacetreeId(masterId);
    message.setWorkerSpacetreeId(newTreeId);
    message.setAction(TreeManagementMessage::Action::CreateNewRemoteTree);
    TreeManagementMessage::sendAndPollDanglingMessages(message, targetRank, _requestMessageTag);

    TreeManagementMessage::receive(message, targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator());
    assertion1(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());

    const peano4::grid::AutomatonState& state = _spacetrees.begin()->_root;
    logDebug("addSpacetree()", "Send state " << state.toString() << " to rank " << targetRank);
    peano4::grid::AutomatonState::send(state, targetRank, _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator());

    TreeManagementMessage::receive(message, targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator());
    assertion1(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
    assertionMsg(false, "MPI build disabled");
#endif
  } else {
    peano4::grid::Spacetree newTree(
      newTreeId,
      masterId,
      _spacetrees.begin()->_root.x,
      _spacetrees.begin()->_root.h,
      _spacetrees.begin()->_root.getInverted()
    );
    tarch::multicore::Lock lock(_semaphore);
    _spacetrees.push_back(std::move(newTree));
  }
  logTraceOut("addSpacetree()");
}

peano4::parallel::SpacetreeSet::TraverseTask::TraverseTask(
  peano4::grid::Spacetree&         tree,
  SpacetreeSet&                    set,
  peano4::grid::GridTraversalObserver& observer,
  bool                             invertTreeTraversalDirectionBeforeWeStart
):
  Task(Task::NoTaskType, Task::DefaultPriority),
  _spacetree(tree),
  _spacetreeSet(set),
  _observer(observer),
  _invertTreeTraversalDirectionBeforeWeStart(invertTreeTraversalDirectionBeforeWeStart) {}

bool peano4::parallel::SpacetreeSet::TraverseTask::run() {
  _spacetreeSet.createObserverCloneIfRequired(_observer, _spacetree._id);
  if (_invertTreeTraversalDirectionBeforeWeStart) {
    _spacetree._root.setInverted(not _spacetree._root.getInverted());
  }
  _spacetree.traverse(*_spacetreeSet._clonedObserver[_spacetree._id], true);
  return false;
}

void peano4::parallel::SpacetreeSet::createObserverCloneIfRequired(peano4::grid::GridTraversalObserver& observer, int treeId) {
  tarch::multicore::Lock lock(_semaphore);
  if (_clonedObserver.count(treeId) == 0) {
    _clonedObserver.insert(std::pair<int, peano4::grid::GridTraversalObserver*>(treeId, observer.clone(treeId)));
  }
  if (_clonedObserver[treeId] == nullptr) {
    _clonedObserver[treeId] = observer.clone(treeId);
  }
  assertion2(_clonedObserver.count(treeId) == 1 and _clonedObserver[treeId] != nullptr, treeId, _clonedObserver.count(treeId));
}

void peano4::parallel::SpacetreeSet::streamLocalVertexInformationToMasterThroughVerticalStacks([[maybe_unused]] int spacetreeId, [[maybe_unused]] int parentId, const std::set<int>& /*joiningIds*/) {
  logTraceInWith2Arguments("streamLocalVertexInformationToMasterThroughVerticalStacks()", spacetreeId, parentId);

  /*
  const int destinationRank  = Node::getInstance().getRank( parentId );
  const int sourceRank       = Node::getInstance().getRank( spacetreeId );
  const int destinationStack = Node::getInstance().getInputStackNumberForVerticalDataExchange( spacetreeId );
  const int sourceStack      = Node::getInstance().getOutputStackNumberForVerticalDataExchange( parentId );
  if (
    destinationRank != tarch::mpi::Rank::getInstance().getRank()
    and
    sourceRank      == tarch::mpi::Rank::getInstance().getRank()
    and
    not peano4::grid::Spacetree::_vertexStack.getForPush( peano4::maps::StackKey(spacetreeId,sourceStack) )->empty()
  ) {
    assertion(false);
    const int tag = Node::getInstance().getGridDataExchangeTag( sourceSpacetreeId, destinationSpacetreeId,
  Node::ExchangeMode::SendVerticalData ); logInfo( "exchangeStacksSynchronously()", "send stack " << sourceStack << "
  from tree " << sourceSpacetreeId << " to rank " << destinationRank <<
      ": " << stackContainer[ peano4::grid::Spacetree::StackKey(destinationSpacetreeId,destinationStack) ].toString()
    );

    tarch::mpi::IntegerMessage message( stackContainer[ peano4::grid::Spacetree::StackKey(sourceSpacetreeId,sourceStack)
  ].size() );
    message.send(destinationRank,tag,false,tarch::mpi::IntegerMessage::ExchangeMode::NonblockingWithPollingLoopOverTests);

    stackContainer[ peano4::grid::Spacetree::StackKey(sourceSpacetreeId,sourceStack) ].startSend(destinationRank,tag);
    stackContainer[ peano4::grid::Spacetree::StackKey(sourceSpacetreeId,sourceStack) ].finishSendOrReceive();
  }

  // habe ich die Stacks immer noch oder missbrauch ich die zur Zeit fuer MPI sends und receives?
  if (
    destinationRank == tarch::mpi::Rank::getInstance().getRank()
    and
    sourceRank      == tarch::mpi::Rank::getInstance().getRank()
    and
    not peano4::grid::Spacetree::_vertexStack.getForPush( spacetreeId,sourceStack )->empty()
  ) {
    logDebug( "exchangeAllVerticalDataExchangeStacks()", "stream content of stack " << sourceStack << " on tree " <<
  spacetreeId << " into stack " << destinationStack << " of tree " << parentId << " as source tree is joining" );
    peano4::grid::Spacetree::_vertexStack.getForPush(parentId,destinationStack)->clone(
  *peano4::grid::Spacetree::_vertexStack.getForPop(spacetreeId,sourceStack) );
    peano4::grid::Spacetree::_vertexStack.getForPop(spacetreeId,sourceStack)->clear();
    peano4::grid::Spacetree::_vertexStack.getForPop(parentId,destinationStack)->reverse();
  }
*/

  logTraceOut("streamLocalVertexInformationToMasterThroughVerticalStacks()");
}


void peano4::parallel::SpacetreeSet::deleteAllStacks(peano4::grid::GridTraversalObserver& observer, int spacetreeId) {
  logTraceInWith1Argument("deleteAllStacks(int)", spacetreeId);
  deleteAllStacks(peano4::grid::Spacetree::_vertexStack, spacetreeId);
  createObserverCloneIfRequired(observer, spacetreeId);
  _clonedObserver[spacetreeId]->deleteAllStacks();
  logTraceOut("deleteAllStacks(int)");
}


void peano4::parallel::SpacetreeSet::exchangeVerticalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer) {
  logTraceIn("exchangeVerticalDataBetweenTrees()");

  for (auto& p : _spacetrees) {
    logDebug(
      "exchangeVerticalDataBetweenTrees(GridTraversalObserver&)",
      "Manage data transfer of tree " << p._id << " in state " << peano4::grid::toString(p._spacetreeState)
    );

    streamLocalVertexInformationToMasterThroughVerticalStacks(p._id, p._masterId, p._joining);
    exchangeAllVerticalDataExchangeStacks(peano4::grid::Spacetree::_vertexStack, p._id, p._masterId);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->exchangeAllVerticalDataExchangeStacks(p._masterId);
  }

  logDebug(
    "exchangeVerticalDataBetweenTrees(GridTraversalObserver&)",
    "All local data exchange realised, all MPI message exchange triggered. Wait for MPI communication."
  );

  for (auto& p : _spacetrees) {
    finishAllOutstandingSendsAndReceives(peano4::grid::Spacetree::_vertexStack, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut("exchangeVerticalDataBetweenTrees()");
}


void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(peano4::grid::GridTraversalObserver& observer) {
  logTraceInWith1Argument("streamDataFromSplittingTreesToNewTrees()", _spacetrees.size());

  for (auto& parent : _spacetrees) {
      const int sourceStack = grid::PeanoCurve::getInputStackNumber(parent._root);
    for (auto& worker : parent._hasSplit) {
      const int destinationStack = Node::getOutputStackNumberForVerticalDataExchange(worker);
      assertion3(peano4::grid::Spacetree::_vertexStack.getForPush(parent._id, destinationStack)->empty(), parent._id, destinationStack, sourceStack);
      peano4::grid::Spacetree::_vertexStack.getForPush(parent._id, destinationStack)->clone(*peano4::grid::Spacetree::_vertexStack.getForPop(parent._id, sourceStack));
      streamDataFromSplittingTreeToNewTree(peano4::grid::Spacetree::_vertexStack, parent._id, worker);
      createObserverCloneIfRequired(observer, parent._id);
      _clonedObserver[parent._id]->streamDataFromSplittingTreeToNewTree(worker);
    }
  }

  for (auto& p : _spacetrees) {
    if (p._spacetreeState == peano4::grid::SpacetreeState::EmptyRun) {
      streamDataFromSplittingTreeToNewTree(peano4::grid::Spacetree::_vertexStack, p._masterId, p._id);
      createObserverCloneIfRequired(observer, p._masterId);
      _clonedObserver[p._masterId]->streamDataFromSplittingTreeToNewTree(p._id);
    }
  }

  for (auto& p : _spacetrees) {
    createObserverCloneIfRequired(observer, p._id);
    finishAllOutstandingSendsAndReceives(peano4::grid::Spacetree::_vertexStack, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut("streamDataFromSplittingTreesToNewTrees()");
}


void peano4::parallel::SpacetreeSet::exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer) {
  logTraceIn("exchangeHorizontalDataBetweenTrees()");

  for (auto& p : _spacetrees) {
    logDebug(
      "exchangeHorizontalDataBetweenTrees(GridTraversalObserver&)",
      "manage data transfer of tree " << p._id << " in state " << peano4::grid::toString(p._spacetreeState)
    );
    exchangeAllHorizontalDataExchangeStacks(peano4::grid::Spacetree::_vertexStack, p._id, true);
    exchangeAllPeriodicBoundaryDataStacks(peano4::grid::Spacetree::_vertexStack, p._id);

    createObserverCloneIfRequired(observer, p._id);

    _clonedObserver[p._id]->exchangeAllHorizontalDataExchangeStacks(true);
    _clonedObserver[p._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  logDebug(
    "exchangeHorizontalDataBetweenTrees(GridTraversalObserver&)",
    "all local data exchange realised, all MPI message exchange triggered. Wait for MPI to terminate"
  );

  for (auto& p : _spacetrees) {
    finishAllOutstandingSendsAndReceives(peano4::grid::Spacetree::_vertexStack, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut("exchangeHorizontalDataBetweenTrees()");
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
  logTraceIn("traverse()");

  // if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
  //   peano4::parallel::Node::getInstance().continueToRun();
  // }
  logDebug("traverse()", "Start new grid sweep");

  tarch::mpi::Rank::getInstance().barrier([&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    answerQuestions();
  });
  logDebug("traverse()", "Rank has passed barrier");

  Node::getInstance().createPendingCommunicators();

  std::vector<tarch::multicore::Task*> primaryTasks;
  std::vector<tarch::multicore::Task*> secondaryTasks;
  std::vector<tarch::multicore::Task*> tertiaryTasks;

  for (auto& p : _spacetrees) {
    switch (p._spacetreeState) {
    case peano4::grid::SpacetreeState::NewRoot:
    case peano4::grid::SpacetreeState::JoinTriggered:
    case peano4::grid::SpacetreeState::Joining:
      logDebug(
        "traverse()",
        "Issue task to traverse tree " << p._id << " in the primary tree set " << p.toString()
      );
      primaryTasks.push_back(new TraverseTask(p, *this, observer, false));
      break;
    case peano4::grid::SpacetreeState::Running:
      if (p._joining.empty()) {
        logDebug(
          "traverse()",
          "Issue task to traverse tree " << p._id << " in the primary tree set " << p.toString()
        );
        primaryTasks.push_back(new TraverseTask(p, *this, observer, false));
      } else {
        logDebug(
          "traverse()",
          "Issue task to traverse tree "
            << p._id << " in the third tree set as it joins in other tree: " << p.toString()
        );
        tertiaryTasks.push_back(new TraverseTask(p, *this, observer, false));
      }
      break;
    case peano4::grid::SpacetreeState::EmptyRun:
      logDebug(
        "traverse()",
        "Issue task to traverse tree " << p._id << " in secondary tree set as empty tree and in third set as new tree"
      );
      secondaryTasks.push_back(new TraverseTask(p, *this, observer, true));
      tertiaryTasks.push_back(new TraverseTask(p, *this, observer, false));
      break;
    case peano4::grid::SpacetreeState::Joined:
    case peano4::grid::SpacetreeState::NewFromSplit:
      assertionMsg("Should not happen", false);
      break;
    }
  }

#ifdef WITH_OPENMP
#pragma omp parallel
  {
#pragma omp master
#endif
    {
      if (not primaryTasks.empty()) {
        tarch::multicore::spawnTasks(primaryTasks);
      }

      streamDataFromSplittingTreesToNewTrees(observer);
      exchangeVerticalDataBetweenTrees(observer);

      if (not secondaryTasks.empty()) {
        tarch::multicore::spawnTasks(secondaryTasks);
      }

      exchangeVerticalDataBetweenTrees(observer);

      if (not tertiaryTasks.empty()) {
        tarch::multicore::spawnTasks(tertiaryTasks);
      }

      exchangeHorizontalDataBetweenTrees(observer);
    }
#ifdef WITH_OPENMP
  }
#endif

  cleanUpTrees(observer);
  createNewTrees();
  deleteClonedObservers();

  logTraceOut("traverse()");
}


void peano4::parallel::SpacetreeSet::cleanUpTrees(peano4::grid::GridTraversalObserver& observer) {
  logTraceIn("cleanUpTrees()");
  for (auto p = _spacetrees.begin(); p != _spacetrees.end();) {
    /*
        else if (
          p->getGridStatistics().getCoarseningHasBeenVetoed()
          and
          p->mayJoinWithMaster()
          and
        ) {
          logInfo( "traverse(Observer)", "trigger join of tree " << p->_id << " with its master tree " << p->_masterId
     << " to enable further grid erases"); join(p->_id);
     //
          p->_masterId>=0
          and
          getSpacetree(p->_masterId).mayJoinWithWorker()
        }
    */
    if (p->_spacetreeState == peano4::grid::SpacetreeState::Joined) {
      logInfo("traverse(Observer)", "Tree " << p->_id << " has successfully joined");
      deleteAllStacks(observer, p->_id);
      Node::getInstance().deregisterId(p->_id);
      p = _spacetrees.erase(p);
    } else if (p->mayJoinWithMaster() and p->getGridStatistics().numberOfLocalUnrefinedCells == 0) {
      logInfo("traverse(Observer)", "Remove empty tree " << p->_id << " with master " << p->_masterId);
      deleteAllStacks(observer, p->_id);
      Node::getInstance().deregisterId(p->_id);

      if (Node::getInstance().getRank(p->_masterId) != tarch::mpi::Rank::getInstance().getRank()) {
        logDebug(
          "traverse(Observer)", "Parent tree " << p->_masterId << " is not local on this rank. Remove child reference"
        );
#ifdef WITH_MPI
        TreeManagementMessage message(
          p->_masterId, p->_id, TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty
        );
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
        assertion1(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
        assertionMsg(false, "Branch may not be entered");
#endif
      } else {
        logDebug(
          "traverse(Observer)", "Parent tree " << p->_masterId << " is local on this rank. Remove child reference"
        );
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
          "traverse(Observer)",
          "Join tree "
            << p->_id << " as it is deteriorated (encodes no hierarchical data) while master " << p->_masterId
            << " resides on same rank and can't coarsen"
        );
        logError("traverse(Observer)", "Not implemented yet");
        // p->joinWithMaster();
        // getSpacetree(p->_masterId).joinWithWorker(p->_id);
      } else if (localRank == masterRank) {
        logDebug(
          "traverse(Observer)",
          "Tree " << p->_id << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master"
        );
      } else {
        // @todo: Aber nur, wenn es noch andere Baeume auf diesem Rank gibt
        // @todo erste Meldung info
        logDebug(
          "cleanUpTrees()", "I should merge tree " << p->_id << " to reduce synchronisation: " << p->toString()
        );
        logDebug("traverse(Observer)", "Not implemented yet");
      }
    }
    p++;
  }
  logTraceOut("cleanUpTrees()");
}


const peano4::grid::GridStatistics& peano4::parallel::SpacetreeSet::getGridStatistics(int treeId) const {
  assertion2(isLocalSpacetree(treeId), treeId, tarch::mpi::Rank::getInstance().getRank());
  return getSpacetree(treeId).getGridStatistics();
}

peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  grid::GridStatistics result;
  bool any = false;

  for (const auto& tree : _spacetrees) {
    if (tree._spacetreeState == grid::SpacetreeState::NewFromSplit) continue;
    if (!any) {
      result = tree._statistics;
      any = true;
    } else {
      result += tree._statistics;
    }
  }

  return any ? result : grid::GridStatistics{};
}

bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  for (auto& p : _spacetrees) {
    if (p._id == treeId)
      return true;
  }
  return false;
}


std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;

  for (auto& p : _spacetrees) {
    result.insert(p._id);
  }

  return result;
}


bool peano4::parallel::SpacetreeSet::split(int treeId, const grid::SplitInstruction& instruction, int targetRank) {
  logTraceInWith3Arguments("split()", treeId, instruction, targetRank);
  peano4::grid::Spacetree& tree = getSpacetree(treeId);

  if (tree.maySplit()) {
    int newSpacetreeId = -1;

    if (tarch::mpi::Rank::getInstance().getRank() != targetRank) {
#ifdef WITH_MPI
      logDebug("split()", "Request new tree on rank " << targetRank);
      peano4::parallel::TreeManagementMessage requestMessage;
      requestMessage.setMasterSpacetreeId(treeId);
      requestMessage.setWorkerSpacetreeId(-1);
      requestMessage.setAction(peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree);
      TreeManagementMessage::sendAndPollDanglingMessages(requestMessage, targetRank, _requestMessageTag);

      logDebug("split()", "Message " << requestMessage.toString() << " sent - wait for answer");

      peano4::parallel::TreeManagementMessage answerMessage;
      TreeManagementMessage::receiveAndPollDanglingMessages(answerMessage, targetRank, getAnswerTag(treeId));
      assertion(answerMessage.getAction() == TreeManagementMessage::Action::Acknowledgement);
      newSpacetreeId = answerMessage.getWorkerSpacetreeId();
#else
      newSpacetreeId = -1;
      assertionMsg(false, "Can't split into tree on a different rank if not compiled with parallel support");
#endif
    } else {
      newSpacetreeId = peano4::parallel::Node::getInstance().reserveId(
        peano4::parallel::Node::getInstance().getRank(treeId), treeId
      );
    }

    if (newSpacetreeId >= 0) {
      tree.split(newSpacetreeId, instruction);
      logDebug(
        "split()",
        "Trigger split of tree " << treeId << " into tree " << newSpacetreeId << " on rank " << targetRank << " with instruction " << instruction
      );
      logTraceOutWith1Argument("split()", true);
      return true;
    }
  }

  logTraceOutWith1Argument("split()", false);
  return false;
}


peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int id) {
  for (auto& p : _spacetrees) {
    if (p._id == id)
      return p;
  }
  assertion3(false, "no spacetree found", id, tarch::mpi::Rank::getInstance().getRank());
  return *_spacetrees.begin(); // just here to avoid warning
}


const peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int id) const {
  for (auto& p : _spacetrees) {
    if (p._id == id)
      return p;
  }
  assertion3(false, "no spacetree found", id, tarch::mpi::Rank::getInstance().getRank());
  return *_spacetrees.begin(); // just here to avoid warning
}
