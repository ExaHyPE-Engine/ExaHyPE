// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/Node.h"
#include "tarch/MemoryUsage.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
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
  int                        sendTree,
  int                        recvTree,
  const grid::PeriodicStamp& stamp,
  int                        depositStack,
  grid::GridVertex           data
) {
  _crossRankPeriodicGridVertexStaging.push_back(
    grid::CrossRankPeriodicEntry<grid::GridVertex>{sendTree, recvTree, stamp, depositStack, data}
  );
}

std::vector<peano4::grid::CrossRankPeriodicEntry<peano4::grid::GridVertex>>&
peano4::parallel::SpacetreeSet::getCrossRankPeriodicStaging() {
  return _crossRankPeriodicGridVertexStaging;
}

void peano4::parallel::SpacetreeSet::clearCrossRankPeriodicStaging() { _crossRankPeriodicGridVertexStaging.clear(); }

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::parallel::SpacetreeSet::getParallelExchangeStack(
  int                                                treeId,
  int                                                slot,
  peano4::grid::StackTopology::ParallelExchangeStack which
) {
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
  _answerMessageTag =
    tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - answer message", MAX_SPACETREES_PER_RANK);
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

  _periodicBC = periodicBC;

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _spacetrees.emplace_back(0, offset, width, periodicBC);
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

/**
 * @return Tag that one should use to answer one particular spacetree
 */
int peano4::parallel::SpacetreeSet::getAnswerTag(int targetSpacetreeId) const {
  return _answerMessageTag + Node::getInstance().getLocalTreeId(targetSpacetreeId);
}

/**
 * Run through the set of unanswered questions and, well, answer them.
 *
 * The following messages are received/handled:
 *
 * - Action::RequestNewRemoteTree (invoked by split())
 * - Action::CreateNewRemoteTree (invoked by addSpacetree(int,int))
 * - Action::RemoveChildTreeFromBooksAsChildBecameEmpty (invoked by cleanUpTrees())
 *
 * split() is something any rank can trigger at any time. Most default load
 * balancers call it throughout the end of the grid sweep. addSpacetree() is
 * called by the set through createNewTrees() just before all observers are
 * deleted and the traversal terminates. cleanUpTrees() just arises before
 * that one.
 *
 * Not all of these messages can be answered at any point of the local grid
 * sweeps. That is, we may never add a tree to the local tree collection while
 * we are right in the middle of traversals on this rank, e.g.
 * We may not insert or
 * remove trees from the global data structure while we are still traversing
 * some local trees or run some data exchange. I originally tried to protect
 * the whole code by a _state==SpacetreeSetState::Waiting check, i.e. to make
 * the set react only while we are not doing anything anyway. That did
 * not work properly, as tree requests for example are to be handled
 * immediately. So what I do now is that I pipe all requests into a vector.
 * Then, I run through the vector and, depending on the set state, do answer
 * messages or leave them in the queue for the time being.
 *
 * This approach leads to errors whenever a message send-out is followed by
 * a second message that provides further details. The first message might be
 * buffered locally, and, as we can't answer the first one immediately, the
 * second message (of another datatype) will be interpreted as yet another
 * request. So that means that every single message exchange with the set
 * always has to be follow a send-acknowledge pattern.
 *
 * <h2> Action::RequestNewRemoteTree </h2>
 *
 * Can be answered any time, as it literally just books a new number but
 * nothing more happens at this point. The acknowledgement message carries
 * the new tree's number.
 *
 * <h2> Action::CreateNewRemoteTree </h2>
 *
 * This message triggers the insertation of a new tree into the local set of
 * spacetrees. We thus may handle it only at the end of a
 * traversal. The message logically consists of two parts: The master of a
 * new tree (which has first used Action::RequestNewRemoteTree to get a
 * new tree's number) sends out the create message, waits for the
 * acknowledgement and then sends out the tree state, to the rank hosting the
 * new tree can actually create the data structure. This last step is followed
 * by an acknowledge message which carries no particular information. That
 * is, this message exchange belongs to the one-way information flow, but we
 * have one acknowledgement message to say "go ahead", and one message to
 * say "ok, we got it".
 *
 * <h2> Action::RemoveChildTreeFromBooksAsChildBecameEmpty </h2>
 *
 * This is a simple one though we have again to ensure that it is handled
 * only once we have finished the local traversals and thus can safely
 * manipulate the local spacetree.
 *
 * <h2> Data consistency </h2>
 *
 * In this routine, we have to be very careful with the data consistency. While
 * we are running through the set of unanswered messages, new ones might drop
 * in. However, C++ is not very happy if a vector is added further elements
 * while we are traversing it (in theory, it might happen that it is expanded
 * and thus copied). So what I do is something different: I first run through
 * the vector and take those out that I know that I can handle. Then I handle
 * these guys and return - well-aware that meanwhile further messages might
 * have dropped in. Which is something I don't care, as I rely on the calling
 * code just to invoke answer again if it is important for the code's progress.
 *
 */
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
      peano4::grid::AutomatonState::receive(
        state,
        p.getSenderRank(),
        _stateMessageTag,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );

      _spacetrees.emplace_back(
        p.getWorkerSpacetreeId(),
        p.getMasterSpacetreeId(),
        state.x,
        state.h,
        state.getInverted(),
        _periodicBC
      );
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
        "Learned that remote child tree " << p.getWorkerSpacetreeId() << " of local tree " << p.getMasterSpacetreeId()
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

/**
 * We poll the tree management messages.
 *
 * Messages are not answered directly. Instead, we buffer them in
 * _unansweredMessages and hand over to replyToUnansweredMessages().
 *
 * See the description in mpi.h for a howto where this routine is
 * used and how to use it.
 */
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

/**
 * Adds a new spacetree to the set. The responsibility goes over to the
 * set. The operation clones the original spacetree handed in into a new
 * spacetree with the id newTreeId.
 *
 * <h2> Local node </h2>
 *
 * If the new tree will be a local tree, we simply add a new tree object.
 * The Spacetree's constructor takes care of all the "cloning". Clone here
 * means a setup. We traverse this tree afterwards separately to stream all
 * data in.
 *
 * <h2> Distributed memory <h2>
 *
 * In a distributed memory environment, we have to break up the creation
 * into a set of sends forth and back through TreeManagementMessages. This
 * routine is the one that sends out the requests.
 *
 * <h2> Multithreading </h2>
 *
 * I may not globally lock this routine, as I otherwise would block the
 * createObserverCloneIfRequired(). So I literally "only" protect the
 * actual push back in the vector.
 *
 * <h2> Call sequence </h2>
 *
 * The operation is used by createNewTrees() .
 */
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

    TreeManagementMessage::receive(
      message,
      targetRank,
      getAnswerTag(masterId),
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());

    const peano4::grid::AutomatonState& state = _spacetrees.begin()->_root;
    logDebug("Send state " << state.toString() << " to rank " << targetRank);
    peano4::grid::AutomatonState::send(
      state,
      targetRank,
      _stateMessageTag,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );

    TreeManagementMessage::receive(
      message,
      targetRank,
      getAnswerTag(masterId),
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    assertion(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
#else
    assertion(false, "MPI build disabled");
#endif
  } else {
    // The worker inherits its master's bounding box, traversal inversion, and
    // periodic configuration; a worker with a diverging periodic setup could
    // not detect its periodic boundaries.
    const peano4::grid::Spacetree& master = getSpacetree(masterId);
    tarch::multicore::Lock         lock(_semaphore);
    _spacetrees.emplace_back(
      newTreeId,
      masterId,
      master._root.x,
      master._root.h,
      master._root.getInverted(),
      master._periodicBC
    );
    registerLocalSpacetree(_spacetrees.back());
  }
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
 * Quick lookup whether an observer clone for this tree id does already exist.
 * If not, we create one quickly.
 *
 * \section Multithreading
 *
 * This operation uses a lock on the semaphore to ensure that no two threads
 * insert an observer into the global table at the same time.
 */
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

/**
 * Copy the data from a splitting tree onto its new workers
 *
 * When we split a tree, we realise this split in two grid sweeps where the second
 * sweep breaks up the traversal into three logical substeps. In the first sweep, the
 * splitting master tells everybody around that it will split. No split is done though.
 * After this first sweep, the grid data structure is replicated on the new worker.
 * Please note that we only replicate the grid, i.e. there's no user information
 * associated with it yet.
 *
 * In the second sweep, the master still "owns" all data, i.e. it receives all
 * boundary data and merges it in. However, it does not send out boundary data anymore.
 * Instead, it takes all data that should go over to the new worker and dumps it in the
 * vertical data exchange stack. That's part one of the second sweep. Part two means
 * that the new worker runs through its mesh for the first time and incorporates all
 * the streamed user data. After that, we have to run through the grid one more time
 * on the worker (without any action) to get all the data into the right order.
 *
 * ## Vertex grid data
 *
 * The copying of the whole tree data is literally a copying of the output stack of
 * the master. It happens after the master has finished its splitting traversal.
 * peano4::grid::Spacetree::traverse() does invert the traversal direction in an
 * epilogue automatically. Therefore, by the time we hit this routine, we have to
 * copy over the input stack - it is already in the right order for the "counter"-
 * traversal.
 *
 * We always have to invoke the data exchange for both the master and the worker, i.e.
 * we call it twice. This way, we invoke the data exchange on the destination rank
 * (MPI receive) and on the source rank (MPI send). For a single-node split, the
 * second invocation degenerates to nop automatically. See streamDataFromSplittingTreeToNewTree()
 * which implements a simple emptyness check.
 *
 * ## User data
 *
 * It is only the vertex grid data that is copied over in one rush prior to the splitting
 * state. User data is streamed within the splitting state. See Spacetree::sendUserData().
 * The latter routine dumps data in the vertical data exchange stacks, and these stacks
 * subsequently are then transferred over by the present routine, too.
 *
 * ## Stack types
 *
 * All the routines copy over the stream stack data, i.e. the stacks for the vertical
 * data exchange. It is hence important to recognise that the normal stacks are not
 * transferred.
 *
 * @see streamDataFromSplittingTreeToNewTree()
 * @see peano4::grid::Spacetree::sendUserData()
 */
void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(
  peano4::grid::GridTraversalObserver& observer
) {
  for (auto& parent : _spacetrees) {
    const int sourceStackNumber = grid::PeanoCurve::getInputStackNumber(parent._root);
    for (auto& worker : parent._hasSplit) {
      const int slot = parent.getNeighbourSlot(worker);
      assertion(slot >= 0, slot, parent._id, worker, parent.toString());

      auto& targetStack =
        parent.getParallelExchangeVertexStack(slot, grid::StackTopology::ParallelExchangeStack::VerticalOutput);
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

/**
 * Collect the periodic descriptors recorded by all local trees during the
 * last traversal and resolve them into the routing table. Under MPI a
 * rendezvous completes the picture across ranks: every descriptor travels to
 * a broker rank derived from its key, and each rank receives back the full
 * record set of exactly the keys it participates in. The routing table and
 * the traffic therefore scale with the rank's own periodic surface, not with
 * the global surface times the rank count.
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

  auto descriptorLess = [](const peano4::grid::PeriodicDescriptor& a, const peano4::grid::PeriodicDescriptor& b) {
    if (a.stamp.key != b.stamp.key) {
      return a.stamp.key < b.stamp.key;
    }
    if (a.stamp.side != b.stamp.side) {
      return static_cast<int>(a.stamp.side) < static_cast<int>(b.stamp.side);
    }
    return a.treeId < b.treeId;
  };
  auto descriptorEqual = [](const peano4::grid::PeriodicDescriptor& a, const peano4::grid::PeriodicDescriptor& b) {
    return a.stamp.key == b.stamp.key and a.stamp.side == b.stamp.side and a.treeId == b.treeId;
  };

  std::sort(producers.begin(), producers.end(), descriptorLess);
  producers.erase(std::unique(producers.begin(), producers.end(), descriptorEqual), producers.end());
  std::sort(consumers.begin(), consumers.end(), descriptorLess);
  consumers.erase(std::unique(consumers.begin(), consumers.end(), descriptorEqual), consumers.end());

  // Topology-change detection. The periodic entities this rank produces or
  // consumes only change on a regrid (split/join/refinement). The routing is
  // rebuilt (and, under MPI, the collective discovery runs) only then; on a
  // stationary grid every sweep just compares against the cached snapshots.
  // The comparison is order-independent (the SFC traversal direction
  // alternates each sweep), hence the sort.
  auto changedVs = [&](
                     const std::vector<peano4::grid::PeriodicDescriptor>& now,
                     const std::vector<peano4::grid::PeriodicDescriptor>& cached
                   ) {
    return now.size() != cached.size() or not std::equal(now.begin(), now.end(), cached.begin(), descriptorEqual);
  };
  int locallyDirty =
    (changedVs(producers, _lastPeriodicProducerDescriptors) or changedVs(consumers, _lastPeriodicConsumerDescriptors))
    ? 1
    : 0;

  // A topology change on any rank can move a local entity's partner, so all
  // ranks must rebuild if any one is dirty. The one-int MPI_MAX reduce is
  // reached in lockstep after the per-sweep barrier, so it cannot deadlock.
  int globallyDirty = locallyDirty;
#ifdef PEANO_WITH_MPI
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks > 1) {
    tarch::mpi::allreduce(
      &locallyDirty,
      &globallyDirty,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
#endif

  if (not globallyDirty) {
    return;
  }
  _lastPeriodicProducerDescriptors = producers;
  _lastPeriodicConsumerDescriptors = consumers;

#ifdef PEANO_WITH_MPI
  // Rendezvous discovery: every descriptor travels to a broker rank derived
  // from its key, the broker collects both sides of each key, and each
  // participating rank gets back exactly the records of the keys it produces
  // or consumes. Traffic and per-rank table size scale with the local
  // periodic surface, not with surface times ranks. The two alltoall rounds
  // are reached in lockstep by all ranks (globallyDirty is a collective
  // verdict), so they cannot deadlock. Tree ids are global, so getRank()
  // tells each side which rank to route to.
  if (numberOfRanks > 1) {
    constexpr int RecordSize = peano4::grid::PackedPeriodicDescriptorSize + 1;
    const auto    comm       = tarch::mpi::Rank::getInstance().getCommunicator();

    auto flatExchange = [&](const std::vector<std::vector<int>>& outgoing) {
      std::vector<int> sendCounts(numberOfRanks, 0);
      std::vector<int> sendDisplacements(numberOfRanks, 0);
      int              totalSend = 0;
      for (int r = 0; r < numberOfRanks; r++) {
        sendCounts[r]        = static_cast<int>(outgoing[r].size());
        sendDisplacements[r] = totalSend;
        totalSend += sendCounts[r];
      }
      std::vector<int> sendBuffer;
      sendBuffer.reserve(totalSend);
      for (const auto& block : outgoing) {
        sendBuffer.insert(sendBuffer.end(), block.begin(), block.end());
      }

      std::vector<int> receiveCounts(numberOfRanks, 0);
      tarch::mpi::alltoall(sendCounts.data(), 1, MPI_INT, receiveCounts.data(), 1, MPI_INT, comm);

      std::vector<int> receiveDisplacements(numberOfRanks, 0);
      int              totalReceive = 0;
      for (int r = 0; r < numberOfRanks; r++) {
        receiveDisplacements[r] = totalReceive;
        totalReceive += receiveCounts[r];
      }
      std::vector<int> receiveBuffer(static_cast<std::size_t>(totalReceive));
      tarch::mpi::alltoallv(
        sendBuffer.data(),
        sendCounts.data(),
        sendDisplacements.data(),
        MPI_INT,
        receiveBuffer.data(),
        receiveCounts.data(),
        receiveDisplacements.data(),
        MPI_INT,
        comm
      );
      return std::make_pair(std::move(receiveBuffer), std::move(receiveDisplacements));
    };

    std::vector<std::vector<int>> toBroker(numberOfRanks);
    auto stage = [&](const std::vector<peano4::grid::PeriodicDescriptor>& list, int kind) {
      for (const auto& descriptor : list) {
        auto& block = toBroker[peano4::grid::periodicKeyBroker(descriptor.stamp.key, numberOfRanks)];
        block.resize(block.size() + RecordSize);
        peano4::grid::packPeriodicDescriptor(descriptor, block.data() + block.size() - RecordSize);
        block.back() = kind;
      }
    };
    stage(producers, 0);
    stage(consumers, 1);

    auto [brokered, brokeredDisplacements] = flatExchange(toBroker);

    // Group the records of each key and remember which ranks reported it;
    // each of those ranks receives the key's complete record set back.
    struct KeyGroup {
      std::vector<std::array<int, RecordSize>> records;
      std::set<int>                            sources;
    };
    std::map<peano4::grid::PeriodicKey, KeyGroup> groups;
    for (int r = 0; r < numberOfRanks; r++) {
      const int begin = brokeredDisplacements[r];
      const int end   = r + 1 < numberOfRanks ? brokeredDisplacements[r + 1] : static_cast<int>(brokered.size());
      for (int i = begin; i + RecordSize <= end; i += RecordSize) {
        const auto descriptor = peano4::grid::unpackPeriodicDescriptor(brokered.data() + i);
        auto&      group      = groups[descriptor.stamp.key];
        auto&      record     = group.records.emplace_back();
        std::copy(brokered.begin() + i, brokered.begin() + i + RecordSize, record.begin());
        group.sources.insert(r);
      }
    }

    std::vector<std::vector<int>> replies(numberOfRanks);
    for (const auto& [key, group] : groups) {
      for (const int source : group.sources) {
        for (const auto& record : group.records) {
          replies[source].insert(replies[source].end(), record.begin(), record.end());
        }
      }
    }

    auto [replied, repliedDisplacements] = flatExchange(replies);

    producers.clear();
    consumers.clear();
    for (std::size_t i = 0; i + RecordSize <= replied.size(); i += RecordSize) {
      auto& target = replied[i + RecordSize - 1] == 0 ? producers : consumers;
      target.push_back(peano4::grid::unpackPeriodicDescriptor(replied.data() + i));
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
    const auto entry = _periodicRouting.find(producer.stamp.key);
    assertion(entry != _periodicRouting.end(), producer.toString());
    const int producerRank = Node::getInstance().getRank(producer.treeId);
    for (const int targetTree : entry->second[peano4::grid::toIndex(peano4::grid::opposite(producer.stamp.side))]) {
      if (Node::getInstance().getRank(targetTree) != producerRank) {
        _crossRankPeriodicChannels.emplace(producer.treeId, targetTree);
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

  for (auto& p : _spacetrees) {
    exchangeAllHorizontalDataExchangeStacks(*this, p._id);
    purgePeriodicBoundaryDataStacks(*this, p._id);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->exchangeAllHorizontalDataExchangeStacks();
    _clonedObserver[p._id]->purgePeriodicBoundaryDataStacks();
  }

  for (auto& p : _spacetrees) {
    exchangeAllPeriodicBoundaryDataStacks(*this, p._id);
    _clonedObserver[p._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  for (auto& p : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, p._id);
    createObserverCloneIfRequired(observer, p._id);
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  exchangeCrossRankPeriodicData(*this);
  observer.exchangeCrossRankPeriodicData();
}

/**
 * I need this routine for technical reasons: Prior to the sweep of trees,
 * I have to identify all of those trees which wanna merge with their
 * workers. This is an analysis I have to do before I actually traverse
 * any worker. Because of this traversal, more trees might denote their
 * workers as joining, so if I query the joining field after the traversal,
 * I intermix newly joining and old joining ranks.
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
 * This operation should be called pretty close towards the end of a traversal.
 * I recommend to invoke it after cleanUpTrees(). It creates new trees and thus
 * might invoke remote method calls on other ranks. It is thus important that
 * all the data exchange locally is done - otherwise we might ask another rank
 * to create a tree for us, while the other rank still waits for boundary data
 * from our side.
 *
 * @see exchangeDataBetweenTrees() for details.
 */
void peano4::parallel::SpacetreeSet::createNewTrees() {
  for (const auto& tree : _spacetrees) {
    for (auto& p : tree._splitting) {
      addSpacetree(tree._id, p);
    }
  }
}

/**
 * @see _clonedObserver
 */
void peano4::parallel::SpacetreeSet::deleteClonedObservers() {
  for (auto& p : _clonedObserver) {
    if (p.second != nullptr) {
      delete p.second;
      p.second = nullptr;
    }
  }
}

/**
 * Invoke traverse on all spacetrees in parallel.
 *
 * <h2> Sequence </h2>
 *
 * It is important that I don't answer to spacetree request messages while I am
 * running through the spacetree set. Some of these request messages add further
 * elements to the set. However, the set should remain invariant while I run
 * through it. So I introduced the _state. See also _unansweredMessages.
 *
 * So when I'm done with the local traversal, I have to ensure that all other sets
 * have got their tree modifications through. That is: If another rank wants to
 * insert a tree, that has to happen in the right traversal. As the other rank
 * will wait for an acknowledgement of the addition, there is no risk that this
 * rank has already proceeded wrongly into the next grid sweep. However, the target
 * rank has to answer the request in the right one and may not proceed to early.
 *
 * So I introduce a barrier. The barrier has to do two things: On the one hand, it
 * has to receive dangling messages. On the other hand, it should answer messages
 * that it has not answered before. In principle, this is indirectly done through
 * receiveDanglingMessages(). However, I played around with running the dangling
 * thing if and only if iprobe tells me that there are new messages. So to be on
 * the safe side, I rather invoke the answer routines manually here.
 *
 * There are two options where to place the barrier: We could add it to the end of
 * traverse() or right after we've received the startup message. There are two different
 * kinds of messages: messages that can be answered straightaway (give me  a new
 * rank) or others which can be answered only in-between iterations. If we add
 * a barrier right at the begin of a traversal, then we are fine, as the typical
 * split (asking for further ranks) arises in-between traversals.
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

/**
 * <h2> Merge process </h2>
 *
 * We make the worker first of all decide whether to merge or not. A
 * worker has the control/knowledge whether to join or not. When a rank
 * wants to join, we first of all run the workers. Then, all data is the
 * join buffers. After that, we sweep through the masters to take up the
 * data. So we may not merge both the worker and the master into their
 * workers at the same time. If we did so, we'd not be able to run all
 * the merging trees in parallel.
 */
void peano4::parallel::SpacetreeSet::cleanUpTrees(peano4::grid::GridTraversalObserver& observer) {
  for (auto p = _spacetrees.begin(); p != _spacetrees.end();) {
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
        TreeManagementMessage message(
          p->_masterId,
          p->_id,
          TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty
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
        logError(
          "Join tree " << p->_id << " as it is deteriorated (encodes no hierarchical data) while master "
                       << p->_masterId << " resides on same rank and can't coarsen"
        );
        logError("Not implemented yet");
      } else if (localRank == masterRank) {
        logDebug(
          "Tree " << p->_id << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master"
        );
      } else {
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

/**
 * Return statistics object for primary spacetree.
 */
peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  grid::GridStatistics result;
  bool                 any = false;

  for (const auto& tree : _spacetrees) {
    if (tree._spacetreeState == grid::SpacetreeState::NewFromSplit) {
      continue;
    }
    if (not any) {
      result = tree._statistics;
      any    = true;
    } else {
      result += tree._statistics;
    }
  }

  return any ? result : grid::GridStatistics{};
}

/**
 * Codes hold one spacetree set per rank. With this routine, you can find
 * out whether a local set contains a particular id.
 */
bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  const int localTreeId = Node::getInstance().getLocalTreeId(treeId);
  if (localTreeId < 0 or localTreeId >= MAX_SPACETREES_PER_RANK) {
    return false;
  }
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  return tree != nullptr and tree->_id == treeId;
}

/**
 * @return Set of ids of local spacetrees
 */
std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;
  for (auto& p : _spacetrees) {
    result.insert(p._id);
  }
  return result;
}

/**
 * Split a local tree.
 *
 * If the target tree shall be stored on the local node, then you pass
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * peano4::parallel::Node::getInstance().getRank(treeId)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * as last argument. Splits on the local node allow Peano to exploit more
 * cores on the node. The details of the total splitting procedure are
 * discussed in @ref peano_domain_decomposition "Peano's generic domain decomposition" discussion.
 *
 *
 * ## Reasons for unsuccessful splits
 *
 * Calling split() is an instruction that Peano might want to ignore. It
 * notably ignores the split instruction if maySplit() on the underlying
 * tree returns false.
 *
 * It also fails to split (and consequently returns false) if the routine
 * peano4::parallel::Node::getInstance().reserveId() is unable to create
 * a new spacetree id.
 *
 *
 * @param treeId  Id of tree that should split, i.e. give away cells to a
 *   new tree
 * @param cells   Number of cells to deploy to a new tree. This count refers
 *   to the status-quo, i.e. if dynamic adaptivity makes an unrefined node
 *   of the tree unfold in the next step, the new @f$ 3^d @f$ children do
 *   count only with one towards this quota.
 * @param targetRank  Rank which should host the new tree.
 *
 * @return Split had been successful
 */
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
      newSpacetreeId =
        peano4::parallel::Node::getInstance().reserveId(peano4::parallel::Node::getInstance().getRank(treeId), treeId);
    }

    if (newSpacetreeId >= 0) {
      tree.split(newSpacetreeId, instruction);
      logDebug(
        "Trigger split of tree " << treeId << " into tree " << newSpacetreeId << " on rank " << targetRank
                                 << " with instruction " << instruction
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
