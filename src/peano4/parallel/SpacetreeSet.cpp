#include "SpacetreeSet.h"
#include "Node.h"

#include "peano4/grid/Spacetree.h"
#include "peano4/grid/grid.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/TraversalObserver.h"


#include "tarch/tarch.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/IntegerMessage.h"

#include "tarch/services/ServiceRepository.h"


tarch::logging::Log peano4::parallel::SpacetreeSet::_log( "peano4::parallel::SpacetreeSet" );

peano4::parallel::SpacetreeSet peano4::parallel::SpacetreeSet::_singleton;


tarch::multicore::BooleanSemaphore  peano4::parallel::SpacetreeSet::_semaphore;


peano4::parallel::SpacetreeSet::SpacetreeSet():
  _state( SpacetreeSetState::Waiting) {
}


peano4::parallel::SpacetreeSet&  peano4::parallel::SpacetreeSet::getInstance() {
  return _singleton;
}


void peano4::parallel::SpacetreeSet::init(
  const tarch::la::Vector<Dimensions,double>&  offset,
  const tarch::la::Vector<Dimensions,double>&  width,
  const std::bitset<Dimensions>&               periodicBC
) {
  _requestMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - request message");
  _answerMessageTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::SpacetreeSet - answer message", Node::MaxSpacetreesPerRank);
  tarch::services::ServiceRepository::getInstance().addService( this, "peano4::parallel::SpacetreeSet" );

  #ifdef Parallel
  assertion4(
    (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees()==1 and tarch::mpi::Rank::getInstance().getRank()==0)
    or
    (peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees()==0 and tarch::mpi::Rank::getInstance().getRank()!=0),
    peano4::parallel::Node::getInstance().getNumberOfRegisteredTrees(),
    offset, width, tarch::mpi::Rank::getInstance().getRank()
  );
  #endif

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    peano4::grid::Spacetree spacetree( offset, width, periodicBC );
    _spacetrees.push_back( spacetree );
  }
}


void peano4::parallel::SpacetreeSet::shutdown() {
  peano4::grid::Spacetree::_vertexStack.clear();
  tarch::services::ServiceRepository::getInstance().removeService( this );
}


peano4::parallel::SpacetreeSet::~SpacetreeSet() {
}


int peano4::parallel::SpacetreeSet::getAnswerTag( int targetSpacetreeId ) const {
  return _answerMessageTag + Node::getInstance().getLocalTreeId(targetSpacetreeId);
}


std::string peano4::parallel::SpacetreeSet::toString( SpacetreeSetState state ) {
  switch (state) {
    case SpacetreeSetState::TraverseTreesAndExchangeData:
      return "traverse-trees-and-exchange-data";
    case SpacetreeSetState::Waiting:
      return "waiting";
  }
  return "undef";
}


void peano4::parallel::SpacetreeSet::answerQuestions() {
  #ifdef Parallel
  std::vector<peano4::parallel::TreeManagementMessage> unansweredMessagesThatIanAnswerNow;

  std::vector<peano4::parallel::TreeManagementMessage>::iterator p = _unansweredMessages.begin();
  while (p!=_unansweredMessages.end()) {
    switch ( p->getAction() ) {
      case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree:
        unansweredMessagesThatIanAnswerNow.push_back( *p );
        p = _unansweredMessages.erase(p);
        break;
      case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty:
      case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTree:
        {
          if ( _state==SpacetreeSetState::Waiting ) {
            unansweredMessagesThatIanAnswerNow.push_back( *p );
            p = _unansweredMessages.erase(p);
          }
          else {
            logDebug( "answerMessages()", "can't answer as I'm in the wrong state" );
            p++;
          }
        }
        break;
      case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
        assertionMsg( false, "should only be passed synchronously and never run through this tag" );
        break;

    }
  }


  for (auto p: unansweredMessagesThatIanAnswerNow) {
    switch ( p.getAction() ) {
      case peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree:
        {
          int newSpacetreeId = peano4::parallel::Node::getInstance().reserveId(
            tarch::mpi::Rank::getInstance().getRank(),  // on current node
            p.getMasterSpacetreeId()              // this is the tree who has requested the new tree
          );

          peano4::parallel::TreeManagementMessage answerMessage;
          answerMessage.setWorkerSpacetreeId( newSpacetreeId );
          answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
          peano4::parallel::TreeManagementMessage::send(answerMessage, p.getSenderRank(), getAnswerTag(p.getMasterSpacetreeId()), tarch::mpi::Rank::getInstance().getCommunicator() );
          logInfo( "receiveDanglingMessages()", "reserved tree id " << newSpacetreeId << " for tree " << p.getMasterSpacetreeId() );
        }
        break;
      case peano4::parallel::TreeManagementMessage::Action::CreateNewRemoteTree:
        {
          assertion( _state==SpacetreeSetState::Waiting );
          peano4::parallel::TreeManagementMessage answerMessage;
          answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
          peano4::parallel::TreeManagementMessage::send(answerMessage, p.getSenderRank(), getAnswerTag(p.getMasterSpacetreeId()), tarch::mpi::Rank::getInstance().getCommunicator() );

          peano4::grid::AutomatonState state;
          peano4::grid::AutomatonState::receive( state, p.getSenderRank(), _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator() );
          peano4::grid::Spacetree newTree(
            p.getWorkerSpacetreeId(),
            p.getMasterSpacetreeId(),
            state.getX(),
            state.getH(),
            state.getInverted()
          );

          _spacetrees.push_back( std::move(newTree) );

          peano4::parallel::TreeManagementMessage::send(answerMessage, p.getSenderRank(), getAnswerTag(p.getMasterSpacetreeId()), tarch::mpi::Rank::getInstance().getCommunicator() );
        }
        break;
      case peano4::parallel::TreeManagementMessage::Action::Acknowledgement:
        assertionMsg( false, "should only be passed synchronously and never run through this tag" );
        break;
      case peano4::parallel::TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty:
        {
          assertion( _state==SpacetreeSetState::Waiting );
          logInfo( "receiveDanglingMessages(...)", "learned that remote child tree " << p.getWorkerSpacetreeId() << " of local tree " << p.getMasterSpacetreeId() << " is degenerated thus had been removed" );
          getSpacetree( p.getMasterSpacetreeId() )._childrenIds.erase(p.getWorkerSpacetreeId());

          peano4::parallel::TreeManagementMessage answerMessage;
          answerMessage.setAction(TreeManagementMessage::Action::Acknowledgement);
          peano4::parallel::TreeManagementMessage::send(answerMessage, p.getSenderRank(), getAnswerTag(p.getWorkerSpacetreeId()), tarch::mpi::Rank::getInstance().getCommunicator() );
        }
        break;
    }
  }
  #else
  assertion(_unansweredMessages.empty());
  #endif
}


void peano4::parallel::SpacetreeSet::receiveDanglingMessages() {
  #ifdef Parallel

  if ( tarch::mpi::Rank::getInstance().isMessageInQueue(_requestMessageTag) ) {
    logTraceIn( "receiveDanglingMessages()" );

    peano4::parallel::TreeManagementMessage message;
    peano4::parallel::TreeManagementMessage::receive( message, MPI_ANY_SOURCE, _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator() );

    _unansweredMessages.push_back( message );

    logDebug( "receiveDanglingMessages()", "received new message " << message.toString() );

    logTraceOut( "receiveDanglingMessages()" );
  }

  answerQuestions();
  #endif
}


void peano4::parallel::SpacetreeSet::addSpacetree( int masterId, int newTreeId ) {
  logTraceInWith2Arguments( "addSpacetree(int,int)", masterId, newTreeId );
  if ( peano4::parallel::Node::getInstance().getRank(masterId)!=peano4::parallel::Node::getInstance().getRank(newTreeId) ) {
    #ifdef Parallel
    const int targetRank = peano4::parallel::Node::getInstance().getRank(newTreeId);

    TreeManagementMessage message;
    message.setMasterSpacetreeId(masterId);
    message.setWorkerSpacetreeId(newTreeId);
    message.setAction( TreeManagementMessage::Action::CreateNewRemoteTree );
    TreeManagementMessage::sendAndPollDanglingMessages( message,  targetRank, _requestMessageTag );

    TreeManagementMessage::receive( message,  targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator() );
    assertion1(message.getAction()==TreeManagementMessage::Action::Acknowledgement, message.toString());

    const int tag = peano4::parallel::Node::getInstance().getGridDataExchangeTag(masterId,newTreeId,peano4::parallel::Node::ExchangeMode::VerticalData);
    peano4::grid::AutomatonState state = _spacetrees.begin()->_root;

    logDebug( "addSpacetree(int,int)", "send state " << state.toString() << " to rank " << targetRank << " via tag " << tag );
    peano4::grid::AutomatonState::send( state, targetRank, _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator() );

    TreeManagementMessage::receive( message,  targetRank, getAnswerTag(masterId), tarch::mpi::Rank::getInstance().getCommunicator() );
    assertion1(message.getAction()==TreeManagementMessage::Action::Acknowledgement, message.toString());
    #else
    assertionMsg( false, "should never enter this branch without -DParallel" );
    #endif
  }
  else {
    peano4::grid::Spacetree newTree(
      newTreeId,
      masterId,
      _spacetrees.begin()->_root.getX(),
      _spacetrees.begin()->_root.getH(),
      _spacetrees.begin()->_root.getInverted()
    );
    tarch::multicore::Lock lock( _semaphore );
    _spacetrees.push_back( std::move(newTree) );
  }
  logTraceOut( "addSpacetree(int,int)" );
}


peano4::parallel::SpacetreeSet::TraverseTask::TraverseTask(
  peano4::grid::Spacetree&          tree,
  SpacetreeSet&                     set,
  peano4::grid::TraversalObserver&  observer,
  bool                              invertTreeTraversalDirectionBeforeWeStart
):
  _spacetree(tree),
  _spacetreeSet(set),
  _observer( observer ),
  _invertTreeTraversalDirectionBeforeWeStart(invertTreeTraversalDirectionBeforeWeStart) {
}


bool peano4::parallel::SpacetreeSet::TraverseTask::run() {
  _spacetreeSet.createObserverCloneIfRequired(_observer,_spacetree._id);
  if (_invertTreeTraversalDirectionBeforeWeStart) {
    _spacetree._root.setInverted( not _spacetree._root.getInverted() );
  }
  _spacetree.traverse( *_spacetreeSet._clonedObserver[_spacetree._id], true );
  return false;
}


void peano4::parallel::SpacetreeSet::TraverseTask::prefetch() {
}


void peano4::parallel::SpacetreeSet::createObserverCloneIfRequired(peano4::grid::TraversalObserver& observer, int treeId) {
  tarch::multicore::Lock lock(_semaphore);

  if (_clonedObserver.count(treeId)==0) {
	_clonedObserver.insert( std::pair< int, peano4::grid::TraversalObserver* >(treeId,observer.clone(treeId)) );
  }
  if (_clonedObserver[treeId]==nullptr) {
    _clonedObserver[treeId] = observer.clone(treeId);
  }
  assertion2( _clonedObserver.count(treeId)==1 and _clonedObserver[treeId]!=nullptr, treeId, _clonedObserver.count(treeId) );
}


void peano4::parallel::SpacetreeSet::streamLocalVertexInformationToMasterThroughVerticalStacks(
  int spacetreeId, int parentId,
  const std::set<int>& joiningIds
) {
  logTraceInWith2Arguments( "streamLocalVertexInformationToMasterThroughVerticalStacks(...)", spacetreeId, parentId );

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
    const int tag = Node::getInstance().getGridDataExchangeTag( sourceSpacetreeId, destinationSpacetreeId, Node::ExchangeMode::SendVerticalData );
    logInfo(
      "exchangeStacksSynchronously(...)",
      "send stack " << sourceStack << " from tree " << sourceSpacetreeId << " to rank " << destinationRank <<
      ": " << stackContainer[ peano4::grid::Spacetree::StackKey(destinationSpacetreeId,destinationStack) ].toString()
    );

    tarch::mpi::IntegerMessage message( stackContainer[ peano4::grid::Spacetree::StackKey(sourceSpacetreeId,sourceStack) ].size() );
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
    logDebug( "exchangeAllVerticalDataExchangeStacks(...)", "stream content of stack " << sourceStack << " on tree " << spacetreeId << " into stack " << destinationStack << " of tree " << parentId << " as source tree is joining" );
    peano4::grid::Spacetree::_vertexStack.getForPush(parentId,destinationStack)->clone( *peano4::grid::Spacetree::_vertexStack.getForPop(spacetreeId,sourceStack) );
    peano4::grid::Spacetree::_vertexStack.getForPop(spacetreeId,sourceStack)->clear();
    peano4::grid::Spacetree::_vertexStack.getForPop(parentId,destinationStack)->reverse();
  }
*/

  logTraceOut( "streamLocalVertexInformationToMasterThroughVerticalStacks(...)" );
}


void peano4::parallel::SpacetreeSet::deleteAllStacks( peano4::grid::TraversalObserver&  observer, int spacetreeId ) {
  logTraceInWith1Argument( "deleteAllStacks(int)", spacetreeId );
  deleteAllStacks( peano4::grid::Spacetree::_vertexStack, spacetreeId );
  createObserverCloneIfRequired(observer,spacetreeId);
  _clonedObserver[spacetreeId]->deleteAllStacks();
  logTraceOut( "deleteAllStacks(int)" );
}


void peano4::parallel::SpacetreeSet::exchangeVerticalDataBetweenTrees(peano4::grid::TraversalObserver&  observer) {
  logTraceIn( "exchangeVerticalDataBetweenTrees(...)" );

  for (auto& p: _spacetrees) {
    logDebug( "exchangeVerticalDataBetweenTrees(TraversalObserver&)", "manage data transfer of tree " << p._id << " in state " << peano4::grid::Spacetree::toString(p._spacetreeState) );

    streamLocalVertexInformationToMasterThroughVerticalStacks(
      p._id,
      p._masterId,
      p._joining
    );

    exchangeAllVerticalDataExchangeStacks(
      peano4::grid::Spacetree::_vertexStack,
      p._id,
      p._masterId
    );

    createObserverCloneIfRequired(observer,p._id);

    _clonedObserver[p._id]->exchangeAllVerticalDataExchangeStacks( p._masterId );
  }

  logDebug( "exchangeVerticalDataBetweenTrees(TraversalObserver&)", "all local data exchange realised, all MPI message exchange triggered. Wait for MPI to terminate" );

  for (auto& p: _spacetrees) {
    finishAllOutstandingSendsAndReceives( peano4::grid::Spacetree::_vertexStack, p._id );
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut( "exchangeVerticalDataBetweenTrees(...)" );
}


void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(peano4::grid::TraversalObserver&  observer) {
  logTraceInWith1Argument( "streamDataFromSplittingTreesToNewTrees()", _spacetrees.size() );

  for (auto& parent: _spacetrees) {
    for (auto& worker: parent._hasSplit) {
      const int temporaryOutStackForVertices = Node::getOutputStackNumberForVerticalDataExchange(worker);
      const int sourceStackForVertices       = peano4::grid::PeanoCurve::getInputStackNumber( parent._root );
      peano4::grid::Spacetree::_vertexStack.getForPush(parent._id,temporaryOutStackForVertices)->clone( *peano4::grid::Spacetree::_vertexStack.getForPop(parent._id,sourceStackForVertices) );

      streamDataFromSplittingTreeToNewTree( peano4::grid::Spacetree::_vertexStack, parent._id, worker);

      createObserverCloneIfRequired(observer,parent._id);

      _clonedObserver[parent._id]->streamDataFromSplittingTreeToNewTree( worker );
    }
  }

  for (auto& p: _spacetrees) {
    if (p._spacetreeState==peano4::grid::Spacetree::SpacetreeState::EmptyRun) {
      streamDataFromSplittingTreeToNewTree( peano4::grid::Spacetree::_vertexStack, p._masterId, p._id);

      createObserverCloneIfRequired(observer,p._masterId);

      _clonedObserver[p._masterId]->streamDataFromSplittingTreeToNewTree( p._id );
    }
  }

  for (auto& p: _spacetrees) {
    createObserverCloneIfRequired(observer,p._id);
    finishAllOutstandingSendsAndReceives( peano4::grid::Spacetree::_vertexStack, p._id );
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut( "streamDataFromSplittingTreesToNewTrees()" );
}


void peano4::parallel::SpacetreeSet::exchangeHorizontalDataBetweenTrees(peano4::grid::TraversalObserver&  observer) {
  logTraceIn( "exchangeHorizontalDataBetweenTrees(...)" );

  for (auto& p: _spacetrees) {
    logDebug( "exchangeHorizontalDataBetweenTrees(TraversalObserver&)", "manage data transfer of tree " << p._id << " in state " << peano4::grid::Spacetree::toString(p._spacetreeState) );
    exchangeAllHorizontalDataExchangeStacks( peano4::grid::Spacetree::_vertexStack, p._id, true );
    exchangeAllPeriodicBoundaryDataStacks( peano4::grid::Spacetree::_vertexStack, p._id );

    createObserverCloneIfRequired(observer,p._id);

    _clonedObserver[p._id]->exchangeAllHorizontalDataExchangeStacks( true );
    _clonedObserver[p._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  logDebug( "exchangeHorizontalDataBetweenTrees(TraversalObserver&)", "all local data exchange realised, all MPI message exchange triggered. Wait for MPI to terminate" );

  for (auto& p: _spacetrees) {
    finishAllOutstandingSendsAndReceives( peano4::grid::Spacetree::_vertexStack, p._id );
    _clonedObserver[p._id]->finishAllOutstandingSendsAndReceives();
  }

  logTraceOut( "exchangeHorizontalDataBetweenTrees(...)" );
}


std::set<int> peano4::parallel::SpacetreeSet::getLocalTreesMergingWithWorkers() const {
  std::set<int> result;
  for (const auto& tree: _spacetrees) {
    if (not tree._joining.empty()) {
      result.insert( tree._id );
    }
  }
  return result;
}


void peano4::parallel::SpacetreeSet::createNewTrees() {
  for (const auto& tree: _spacetrees) {
    for (auto& p: tree._splitting) {
      addSpacetree( tree._id, p );
    }
  }
}


void peano4::parallel::SpacetreeSet::deleteClonedObservers() {
  for (auto& p: _clonedObserver) {
    if (p.second != nullptr) {
      delete p.second;
      p.second = nullptr;
    }
  }
}


void peano4::parallel::SpacetreeSet::traverse(peano4::grid::TraversalObserver& observer) {
  logTraceIn( "traverse(TraversalObserver&)" );

  _hasPassedOrderedBarrier.clear();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    peano4::parallel::Node::getInstance().continueToRun();
  }
  logDebug( "traverse(TraversalObserver)", "start new grid sweep" );

  tarch::mpi::Rank::getInstance().barrier(
    [&]() -> void {
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      answerQuestions();
    }
  );
  logDebug( "traverse(TraversalObserver&)", "rank has passed barrier" );

  _state = SpacetreeSetState::TraverseTreesAndExchangeData;

  std::vector< tarch::multicore::Task* > primaryTasks;
  std::vector< tarch::multicore::Task* > secondaryTasks;
  std::vector< tarch::multicore::Task* > tertiaryTasks;

  for (auto& p: _spacetrees) {
    switch (p._spacetreeState) {
      case peano4::grid::Spacetree::SpacetreeState::NewRoot:
      case peano4::grid::Spacetree::SpacetreeState::JoinTriggered:
      case peano4::grid::Spacetree::SpacetreeState::Joining:
        logDebug( "traverse(TraversalObserver&)", "issue task to traverse tree " << p._id << " in the primary tree set " << p.toString() );
        primaryTasks.push_back( new TraverseTask( p, *this, observer, false ));
        break;
      case peano4::grid::Spacetree::SpacetreeState::Running:
        if ( p._joining.empty() ) {
          logDebug( "traverse(TraversalObserver&)", "issue task to traverse tree " << p._id << " in the primary tree set " << p.toString() );
          primaryTasks.push_back( new TraverseTask( p, *this, observer, false ));
        }
        else {
          logDebug( "traverse(TraversalObserver&)", "issue task to traverse tree " << p._id << " in the third tree set as it joins in other tree: " << p.toString() );
          tertiaryTasks.push_back( new TraverseTask( p, *this, observer, false ));
        }
        break;
      case peano4::grid::Spacetree::SpacetreeState::EmptyRun:
        logDebug( "traverse(TraversalObserver&)", "issue task to traverse tree " << p._id << " in secondary tree set as empty tree and in third set as new tree" );
        secondaryTasks.push_back( new TraverseTask( p, *this, observer, true ));
        tertiaryTasks.push_back( new TraverseTask( p, *this, observer, false ));
        break;
      case peano4::grid::Spacetree::SpacetreeState::Joined:
      case peano4::grid::Spacetree::SpacetreeState::NewFromSplit:
        assertionMsg("should not happen", false);
        break;
    }
  }

  // I use this boolean flag from time to time to debug the code.
  const bool runSequentially = false;

  logDebug( "traverse(TraversalObserver&)", "kick off primary tree sweeps: " << primaryTasks.size() << " task(s)" );
  if ( not primaryTasks.empty() ) {
    static int multitasking = peano4::parallel::Tasks::getLocationIdentifier( "peano4::parallel::SpacetreeSet::traverse-1" );
    peano4::parallel::Tasks runs( primaryTasks,
      runSequentially ? peano4::parallel::Tasks::TaskType::Sequential : peano4::parallel::Tasks::TaskType::Task,
      multitasking,true);
  }

  logDebug( "traverse(TraversalObserver&)", "primary tasks (traversals) complete, trigger split data stream if required" );
  streamDataFromSplittingTreesToNewTrees(observer);
  logDebug( "traverse(TraversalObserver&)", "exchange vertical data if required" );
  exchangeVerticalDataBetweenTrees(observer);

  logDebug( "traverse(TraversalObserver&)", "kick off secondary tree sweeps: " << secondaryTasks.size() << " task(s)" );
  if ( not secondaryTasks.empty() ) {
    static int multitasking = peano4::parallel::Tasks::getLocationIdentifier( "peano4::parallel::SpacetreeSet::traverse-2" );
    peano4::parallel::Tasks runs( secondaryTasks,
      runSequentially ? peano4::parallel::Tasks::TaskType::Sequential : peano4::parallel::Tasks::TaskType::Task,
      multitasking,true);
  }

  exchangeVerticalDataBetweenTrees(observer);

  logDebug( "traverse(TraversalObserver&)", "kick off tertiary tree sweeps: " << tertiaryTasks.size() << " task(s)" );
  if ( not tertiaryTasks.empty() ) {
    static int multitasking = peano4::parallel::Tasks::getLocationIdentifier( "peano4::parallel::SpacetreeSet::traverse-3" );
    peano4::parallel::Tasks runs( tertiaryTasks,
      runSequentially ? peano4::parallel::Tasks::TaskType::Sequential : peano4::parallel::Tasks::TaskType::Task,
      multitasking,true);
  }

  // @todo docu
  tarch::multicore::processPendingTasks(0);

  exchangeHorizontalDataBetweenTrees(observer);

  _state = SpacetreeSetState::Waiting;

  cleanUpTrees(observer);
  createNewTrees();

  deleteClonedObservers();
 
  logTraceOut( "traverse(TraversalObserver&)" );
}


void peano4::parallel::SpacetreeSet::cleanUpTrees(peano4::grid::TraversalObserver&  observer) {
  logTraceIn( "cleanUpTrees(...)" );
  for (auto p = _spacetrees.begin(); p!=_spacetrees.end(); ) {
/*
  	else if (
      p->getGridStatistics().getCoarseningHasBeenVetoed()
      and
      p->mayJoinWithMaster()
      and
    ) {
      logInfo( "traverse(Observer)", "trigger join of tree " << p->_id << " with its master tree " << p->_masterId << " to enable further grid erases");
      join(p->_id);
 //
      p->_masterId>=0
      and
      getSpacetree(p->_masterId).mayJoinWithWorker()
    }
*/
    if (
      p->_spacetreeState==peano4::grid::Spacetree::SpacetreeState::Joined
    ) {
      logInfo( "traverse(Observer)", "tree " << p->_id << " has successfully joined" );
      deleteAllStacks( observer, p->_id );
      Node::getInstance().deregisterId(p->_id);
      p = _spacetrees.erase(p);
      p--;
    }
    else if (
      p->mayJoinWithMaster()
      and
      p->getGridStatistics().getNumberOfLocalUnrefinedCells()==0
    )  {
      logInfo( "traverse(Observer)", "remove empty tree " << p->_id );
      deleteAllStacks( observer, p->_id );
      Node::getInstance().deregisterId(p->_id);

      if ( Node::getInstance().getRank(p->_masterId)!=tarch::mpi::Rank::getInstance().getRank() ) {
        logDebug( "traverse(Observer)", "parent tree " << p->_masterId << " is not local on this rank. Remove child reference" );
        #ifdef Parallel
        TreeManagementMessage message( p->_masterId, p->_id, TreeManagementMessage::Action::RemoveChildTreeFromBooksAsChildBecameEmpty );
        TreeManagementMessage::send( message, Node::getInstance().getRank(p->_masterId), _requestMessageTag, tarch::mpi::Rank::getInstance().getCommunicator() );

        TreeManagementMessage::receive( message, Node::getInstance().getRank(p->_masterId), getAnswerTag(p->_id), tarch::mpi::Rank::getInstance().getCommunicator() );
        assertion1( message.getAction()==TreeManagementMessage::Action::Acknowledgement, message.toString() );
        #else
        assertionMsg( false, "branch may not be entered" );
        #endif
      }
      else {
        logDebug( "traverse(Observer)", "parent tree " << p->_masterId << " is local on this rank. Remove child reference" );
        getSpacetree( p->_masterId )._childrenIds.erase(p->_id);
      }

      p = _spacetrees.erase(p);
      p--;
    }
    else if (
      p->mayJoinWithMaster()
    ) {
      const int localRank  = Node::getInstance().getRank( p->_id );
      const int masterRank = Node::getInstance().getRank( p->_masterId );
      if (localRank==masterRank) {
        if (
          getSpacetree(p->_masterId).getGridStatistics().getCoarseningHasBeenVetoed()
          and
          getSpacetree(p->_masterId).mayJoinWithWorker()
        ) {
          // @todo erste Meldung info
          logError( "traverse(Observer)", "join tree " << p->_id << " as it is deteriorated (encodes no hierarchical data) while master " << p->_masterId << " resides on same rank and can't coarsen" );
          logError( "traverse(Observer)", "not implemented yet");
          //p->joinWithMaster();
          //getSpacetree(p->_masterId).joinWithWorker(p->_id);
        }
        else {
          logDebug( "traverse(Observer)", "tree " << p->_id << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master" );
        }
      }
      else {
        // @todo: Aber nur, wenn es noch andere Baeume auf diesem Rank gibt
        // @todo erste Meldung info
        logError( "cleanUpTrees(...)", "I should merge tree " << p->_id << " to reduce synchronisation: " << p->toString() );
        logError( "traverse(Observer)", "not implemented yet");
      }
    }
    p++;
  }
  logTraceOut( "cleanUpTrees(...)" );
}


peano4::grid::GridStatistics  peano4::parallel::SpacetreeSet::getGridStatistics(int treeId) const {
  assertion2( isLocalSpacetree(treeId), treeId, tarch::mpi::Rank::getInstance().getRank() );
  return getSpacetree(treeId).getGridStatistics();
}


peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  logTraceIn( "getGridStatistics()" );
  if (_spacetrees.empty()) {
    peano4::grid::GridStatistics result(
      0,  // __numberOfLocalUnrefinedCells,
      0,  // __numberOfRemoteUnrefinedCells,
      0,  // __numberOfLocalRefinedCells
      0,  // __numberOfRemoteRefinedCells,
      0,  // __stationarySweeps,
      false
    );
    logTraceOutWith1Argument( "getGridStatistics()", result.toString() );
    return result;
  }
  else {
    peano4::grid::GridStatistics result( _spacetrees.begin()->_statistics );
    for (auto&  from: _spacetrees) {
      if (
        from._id != _spacetrees.begin()->_id
        and
        from._spacetreeState!=peano4::grid::Spacetree::SpacetreeState::NewFromSplit
      ) {
        result = result + from._statistics;
      }
    }
    logTraceOutWith1Argument( "getGridStatistics()", result.toString() );
    return result;
  }
}


bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  for (auto& p: _spacetrees) {
    if (p._id==treeId) return true;
  }
  return false;
}


std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;

  for (auto& p: _spacetrees) {
    result.insert( p._id );
  }

  return result;
}


bool peano4::parallel::SpacetreeSet::split(int treeId, int cells, int targetRank) {
  logTraceInWith3Arguments( "split(int,int,int)", treeId, cells, targetRank );
  peano4::grid::Spacetree&  tree = getSpacetree( treeId );

  if ( tree.maySplit() ) {
    int newSpacetreeId = -1;

    if (tarch::mpi::Rank::getInstance().getRank()!=targetRank) {
      #ifdef Parallel
      logDebug( "split(int,int,int)", "request new tree on rank " << targetRank );
      peano4::parallel::TreeManagementMessage requestMessage;
      requestMessage.setMasterSpacetreeId(treeId);
      requestMessage.setWorkerSpacetreeId(-1);
      requestMessage.setAction( peano4::parallel::TreeManagementMessage::Action::RequestNewRemoteTree );
      TreeManagementMessage::sendAndPollDanglingMessages(requestMessage, targetRank, _requestMessageTag);

      logDebug( "split(int,int,int)", "message " << requestMessage.toString() << " sent - wait for answer" );

      peano4::parallel::TreeManagementMessage answerMessage;
      TreeManagementMessage::receiveAndPollDanglingMessages(answerMessage, targetRank, getAnswerTag(treeId));
      assertion(answerMessage.getAction()==TreeManagementMessage::Action::Acknowledgement);
      newSpacetreeId = answerMessage.getWorkerSpacetreeId();
      #else
      newSpacetreeId = -1;
      assertionMsg( false, "can't split into tree on a different rank if not compiled with mpi");
      #endif
    }
    else {
      newSpacetreeId = peano4::parallel::Node::getInstance().reserveId(
        peano4::parallel::Node::getInstance().getRank(treeId),
        treeId
      );
    }

    if (newSpacetreeId>=0) {
      tree.split(newSpacetreeId, cells);
      if ( tarch::getMemoryUsage(tarch::MemoryUsageFormat::MByte)*2 > tarch::getTotalMemory(tarch::MemoryUsageFormat::MByte) ) {
        logWarning(
          "split(int,int)",
          "Peano 4 uses " << tarch::getMemoryUsage(tarch::MemoryUsageFormat::MByte) << " MB on rank " <<
          tarch::mpi::Rank::getInstance().getRank() << " and is asked to split. Total memory is " <<
          tarch::getTotalMemory(tarch::MemoryUsageFormat::MByte) << " MB, i.e. we might run out of memory"
        );
      }

      logInfo(
        "split(int,int)",
        "trigger split of tree " << treeId << " into tree " << newSpacetreeId << " with " << cells << " fine grid cells"
      );
      logTraceOutWith1Argument( "split(int,int,int)", true );
      return true;
    }
  }

  logTraceOutWith1Argument( "split(int,int,int)", false );
  return false;
}


peano4::grid::Spacetree&  peano4::parallel::SpacetreeSet::getSpacetree(int id) {
  for (auto& p: _spacetrees) {
    if (p._id==id) return p;
  }
  assertion3( false, "no spacetree found", id, tarch::mpi::Rank::getInstance().getRank() );
  return *_spacetrees.begin(); // just here to avoid warning
}


const peano4::grid::Spacetree&  peano4::parallel::SpacetreeSet::getSpacetree(int id) const {
  for (auto& p: _spacetrees) {
    if (p._id==id) return p;
  }
  assertion3( false, "no spacetree found", id, tarch::mpi::Rank::getInstance().getRank() );
  return *_spacetrees.begin(); // just here to avoid warning
}


void peano4::parallel::SpacetreeSet::orderedBarrier(const std::string& identifier) {
  logTraceIn( "orderedBarrier()" );

  static tarch::multicore::BooleanSemaphore  semaphore;

  {
    tarch::multicore::Lock lock(semaphore);
    if ( _hasPassedOrderedBarrier.count( identifier) == 0 ) {
      _hasPassedOrderedBarrier.insert( std::pair<std::string,bool>(identifier,false) );
    }
  }

  {
    tarch::multicore::Lock lock(semaphore);
    if ( not _hasPassedOrderedBarrier.at( identifier) ) {
      tarch::mpi::Rank::getInstance().barrier(
        [&]() -> void {
          tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
        }
      );
      _hasPassedOrderedBarrier[ identifier ] = true;
    }
  }

  logTraceOut( "orderedBarrier()" );
}

