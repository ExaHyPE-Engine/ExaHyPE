#include "BooleanSemaphore.h"
#include "Rank.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"


tarch::logging::Log  tarch::mpi::BooleanSemaphore::_log( "tarch::mpi::BooleanSemaphore" );

int tarch::mpi::BooleanSemaphore::_semaphoreCounter(1);


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::_singleton;


void tarch::mpi::BooleanSemaphore::enterCriticalSection() {
  BooleanSemaphoreService::getInstance().acquireLock(_semaphoreNumber);
}


void tarch::mpi::BooleanSemaphore::leaveCriticalSection() {
  BooleanSemaphoreService::getInstance().releaseLock(_semaphoreNumber);
}


tarch::mpi::BooleanSemaphore::BooleanSemaphore( const std::string& identifier ):
  _semaphoreNumber( _semaphoreCounter ) {
  _semaphoreCounter++;
}


tarch::mpi::BooleanSemaphore::~BooleanSemaphore() {
}


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::BooleanSemaphoreService() {
}


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::~BooleanSemaphoreService() {
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::init() {
  _semaphoreTag = tarch::mpi::Rank::reserveFreeTag("global semaphores");
  tarch::services::ServiceRepository::getInstance().addService( this, "tarch::mpi::BooleanSemaphore::BooleanSemaphoreService" );
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::shutdown() {
  tarch::services::ServiceRepository::getInstance().removeService( this );
}


std::string tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::toString() const {
  std::ostringstream msg;
  msg << "(";
  msg << "#sections:" << _map.size();
  for (auto& p: _map) {
    msg << "," << p.first << ":" << (p.second ? "locked" : "free" );
  } 
  msg << ",#requests:" << _pendingLockRequests.size();
  for (auto& p: _pendingLockRequests) {
    msg << "," << "lock " << p.second << " from " << p.first;
  }
  msg << ")";
  return msg.str();
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::serveLockRequests() {
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());

  #ifdef Parallel
  bool servedLockRequest = true;
  while (servedLockRequest) {
    servedLockRequest = false;
    {
      tarch::multicore::Lock lock(_mapAccessSemaphore);
      for (auto request = _pendingLockRequests.begin(); request != _pendingLockRequests.end(); ) {
        addMapEntryLazily(request->second);
        if ( not _map[request->second] ) {
          servedLockRequest = true;
          _map[request->second] = true;
          logDebug( "receiveDanglingMessages()", "locked sempahore " << request->second << " for rank " << request->first << ". state=" << toString() );
          MPI_Send( &(request->second), 1, MPI_INT, request->first, _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator());
          request = _pendingLockRequests.erase(request);
        }
        else {
          request++;
        }
      }
    }
  }
  #endif
}



void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::receiveDanglingMessages() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    #ifdef Parallel
    int flag = false;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);

    if (flag) {
      int number;
      logDebug( "receiveDanglingMessages()", "there's a pending message from " << status.MPI_SOURCE );
      MPI_Recv( &number, 1, MPI_INT, status.MPI_SOURCE, _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE);
      assertion(number!=0);

      logDebug( "receiveDanglingMessages()", "received number " << number << " from rank " << status.MPI_SOURCE );
      if (number>0) {
        tarch::multicore::Lock lock(_mapAccessSemaphore);
        std::pair<int,int> newEntry( status.MPI_SOURCE, number );
        _pendingLockRequests.push_back(newEntry);
        logDebug( "receiveDanglingMessages()", "there are " << _pendingLockRequests.size() << " lock requests in total. state=" << toString() );
      }
      else {
        releaseLock(-number);
      }
    }

    serveLockRequests();
    #endif
  }
}


int tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getNumberOfLockedSemaphores() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    tarch::multicore::Lock lock(_mapAccessSemaphore);
    return _pendingLockRequests.size();
  }
  else {
    return 0;
  }
}


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService& tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance() {
  return _singleton;
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::addMapEntryLazily(int number) {
  assertion1( number>0, number );
  if ( _map.count(number)==0 ) {
    _map.insert( std::pair<int,bool>(number,false) );
  }
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::acquireLock( int number ) {
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    {
      tarch::multicore::Lock lock(_mapAccessSemaphore);
      addMapEntryLazily(number);
    }

    bool gotLock = false;
    while (not gotLock) {
      {
        tarch::multicore::Lock lock(_mapAccessSemaphore);
        if (_map[number]==false) {
          gotLock = true;
          _map[number] = true;
        }
      }
      // only check if not successful, so someone else has the chance to release
      // a lock
      if (not gotLock) {
        receiveDanglingMessages();
      }
    }
    logDebug( "acquireLock()", "successfully acquired lock " << number << " for global master");
  }
  else {
    #ifdef Parallel
    logDebug( "acquireLock()", "have to acquire lock on global master " << tarch::mpi::Rank::getGlobalMasterRank() << " and thus send master a " << number );
           
    MPI_Send( &number, 1, MPI_INT, tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator() );

    logDebug( "acquireLock()", "wait for confirmation from global master rank" );
    MPI_Request request;
    auto  timeOutWarning          = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
    auto  timeOutShutdown         = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
    bool triggeredTimeoutWarning = false;

    MPI_Irecv( &number, 1, MPI_INT, tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator(), &request );
    int flag = 0;
    while ( not flag ) {
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::", "acquireLock(int)", tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::", "acquireLock(int)", tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag );
      }

      receiveDanglingMessages();
      MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
    }
    #else
    assertionMsg( false, "may not happen" );
    #endif
  }
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::releaseLock( int number ) {
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    {
      tarch::multicore::Lock lock(_mapAccessSemaphore);
      assertion( _map.count(number)==1 );
      assertion( _map[number]==true );
      _map[number]=false;
      logDebug( "acquireLock()", "successfully released lock " << number << ". state=" << toString() );
    }

    // It is important to unlock the semaphore, as the serve process itself will reaquire it.
    serveLockRequests();
  }
  else {
    #ifdef Parallel
    number = -number;
    logDebug( "releaseLock()", "send global master " << number << " to release global lock" );
   
    MPI_Send( &number, 1, MPI_INT, tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator() );
    #else
    assertionMsg( false, "may not happen" );
    #endif
  }
}

