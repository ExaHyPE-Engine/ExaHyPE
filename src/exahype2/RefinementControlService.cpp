#include "RefinementControlService.h"
#include "RefinementControl.h"


#include "tarch/services/ServiceRepository.h"
#include "tarch/multicore/Lock.h"


tarch::logging::Log  exahype2::RefinementControlService::_log( "exahype2::RefinementControlService" );

int exahype2::RefinementControlService::_reductionTag( tarch::mpi::Rank::reserveFreeTag("exahype2::RefinementControlService") );

tarch::multicore::BooleanSemaphore  exahype2::RefinementControlService::_semaphore;


exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService result;
  return result;
}


exahype2::RefinementControlService::RefinementControlService() {
  tarch::services::ServiceRepository::getInstance().addService( this, "exahype2::RefinementControlService" );
}


exahype2::RefinementControlService::~RefinementControlService() {
  tarch::services::ServiceRepository::getInstance().removeService( this );
}


void exahype2::RefinementControlService::shutdown() {
}


void exahype2::RefinementControlService::receiveDanglingMessages() {
  #ifdef Parallel
  for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
    if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
      MPI_Status status;
      int        flag;
      MPI_Iprobe(rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);
      if (flag) {
        int numberOfMessages;
        MPI_Get_count( &status, peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(), &numberOfMessages );
// @todo Debug
  logInfo( "receiveDanglingMessages()", "got " << numberOfMessages << " event(s) from rank " << rank );
        std::vector< peano4::grid::GridControlEvent > receiveBuffer;
        receiveBuffer.resize( numberOfMessages );
        MPI_Recv( receiveBuffer.data(), numberOfMessages, peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(), rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE );

        tarch::multicore::Lock lock( _semaphore );
        for (auto p: receiveBuffer) {
    // @todo das ist naetuerlich totaler Kaese
          _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(
            p,
            _maxLifetime + _AdditionalLifetimeReceivedEvents
          ));
    _committedEvents.push_back( p );
        }
  logInfo( "receiveDanglingMessages()", "now hold " << _newEvents.size() << " new events and " << _committedEvents.size() << " committed events (valid for this traversal)" );
      }
    }
  }
  #endif
}


std::string exahype2::RefinementControlService::toString() const {
  std::ostringstream msg;
  msg << "("
      << "#new-events=" << _newEvents.size()
      << ",#committed-events=" << _committedEvents.size()
      << ")";
  return msg.str();
}


void exahype2::RefinementControlService::merge( const RefinementControl& control ) {
  tarch::multicore::Lock lock(_semaphore);
  _newEvents.insert( _newEvents.end(), control._newEvents.begin(), control._newEvents.end() );
}


void exahype2::RefinementControlService::finishStep() {
  _committedEvents.clear();

  int maxLifetime = 0;
  RefinementControl::NewEvents::iterator p = _newEvents.begin();
  while (p!=_newEvents.end()) {
    _committedEvents.push_back( p->first );
    p->second--;

    if (p->second<=0) {
      p = _newEvents.erase(p);
    }
    else {
      maxLifetime = std::max( maxLifetime, p->second );
      p++;
    }
  }

  if (not _committedEvents.empty()) {
    logInfo(
      "finishStep()",
      "activate " << _committedEvents.size() << " refinement/erase instructions (can be taken into account in next grid sweep). Keep " <<
      _newEvents.size() << " event(s) to be re-delivered later (max lifetime " << maxLifetime << ")"
     );
  }

  #ifdef Parallel
  freeAllPendingSendRequests();
  _copyOfCommittedEvents = _committedEvents;
  triggerSendOfCopyOfCommittedEvents();
  #endif
}


#ifdef Parallel
void exahype2::RefinementControl::freeAllPendingSendRequests() {
  while (not _sendRequests.empty()) {
    std::vector<MPI_Request*>::iterator p = _sendRequests.begin();
    while (p!=_sendRequests.end()) {
      if (*p==nullptr) {
        p = _sendRequests.erase(p);

      }
      else {
        int flag;
        MPI_Test( *p, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          *p = nullptr;
        }
        p++;
      }
    }
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  }
}


void exahype2::RefinementControl::triggerSendOfCopyOfCommittedEvents() {
  _sendRequests.clear();

  logInfo( "triggerSendOfCopyOfCommittedEvents()", "share my " << _copyOfCommittedEvents.size() << " event(s) with others" );
  if ( _copyOfCommittedEvents.size()>0 ) {
    _sendRequests = std::vector< MPI_Request* >( tarch::mpi::Rank::getInstance().getNumberOfRanks(), nullptr );
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
      if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
        _sendRequests[rank] = new MPI_Request;
        MPI_Isend( _copyOfCommittedEvents.data(), _copyOfCommittedEvents.size(), peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(), rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), _sendRequests[rank] );
      }
    }
  }
}
#endif

std::vector< peano4::grid::GridControlEvent >  exahype2::RefinementControlService::getGridControlEvents() const {
// @tood Debug
        logInfo( "getGridControlEvents()", "return " << _committedEvents.size() << " grid control events" );
  return _committedEvents;
}


