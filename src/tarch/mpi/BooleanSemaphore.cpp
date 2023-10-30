#include "BooleanSemaphore.h"
#include "Rank.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"

tarch::logging::Log  tarch::mpi::BooleanSemaphore::_log( "tarch::mpi::BooleanSemaphore" );

int tarch::mpi::BooleanSemaphore::_semaphoreCounter(1);

tarch::mpi::BooleanSemaphore::BooleanSemaphoreService  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::_singleton;


void tarch::mpi::BooleanSemaphore::enterCriticalSection() {
  _localRankLockRequestSemaphore.enterCriticalSection();
  BooleanSemaphoreService::getInstance().acquireLock(_semaphoreNumber);
}

void tarch::mpi::BooleanSemaphore::leaveCriticalSection() {
  BooleanSemaphoreService::getInstance().releaseLock(_semaphoreNumber);
  _localRankLockRequestSemaphore.leaveCriticalSection();
}

tarch::mpi::BooleanSemaphore::BooleanSemaphore():
  _semaphoreNumber( _semaphoreCounter ) {
  _semaphoreCounter++;
}


tarch::mpi::BooleanSemaphore::~BooleanSemaphore() {}


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::SemaphoreMapEntry::SemaphoreMapEntry():
  locked(false),
  rankThatLastLocked(-1) {
}


std::string tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::SemaphoreMapEntry::toString() const {
  if (locked) {
    return "(locked,by-rank=" + std::to_string(rankThatLastLocked) + ")";
  }
  else {
    return "(free,last-lock-by-rank=" + std::to_string(rankThatLastLocked) + ")";
  }
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
    msg << "," << p.first << ":" << p.second.toString();
  } 
  msg << ",#requests:" << _pendingLockRequests.size();
  for (auto& p: _pendingLockRequests) {
    msg << "," << "lock " << p.second << " from " << p.first;
  }
  msg << ")";
  return msg.str();
}


bool tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::tryLockSemaphoreOnGlobalMaster( int number, int forRank ) {
  assertion( tarch::mpi::Rank::getInstance().isGlobalMaster() );

  tarch::multicore::Lock lock(_mapAccessSemaphore);

  addMapEntryLazily(number);

  bool gotLock = false;
  assertion( _map.count(number)==1 );
  if ( not _map[number].locked ) {
    gotLock              = true;
    _map[number].locked  = true;
    _map[number].rankThatLastLocked = forRank;
    logDebug( "tryLockSemaphoreOnGlobalMaster(int,int)", "successfully locked semaphore " << number << " for rank " << forRank );
  }

  return gotLock;
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::lockSemaphoreOnGlobalMaster( int number, int forRank ) {
  assertion( tarch::mpi::Rank::getInstance().isGlobalMaster() );

  tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
  tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();

  logDebug( "lockSemaphoreOnGlobalMaster(int,int)", "will try to lock semahpore " << number << " for rank " << forRank );

  bool gotLock      = false;
  bool wroteWarning = false;
  while (not gotLock) {
    gotLock = tryLockSemaphoreOnGlobalMaster(number,forRank);

    if ( not wroteWarning and tarch::mpi::Rank::getInstance().exceededTimeOutWarningThreshold() ) {
      wroteWarning = true;
      logWarning( "lockSemaphoreOnGlobalMaster()", "semaphore " << number << " is locked by " << _map[number].rankThatLastLocked << " and therefore cannot be locked for " << forRank );
    }

    receiveDanglingMessages();
  }
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::unlockSemaphoreOnGlobalMaster( int number, int forRank ) {
  assertion( tarch::mpi::Rank::getInstance().isGlobalMaster() );
  assertion2( number>0, number, forRank );

  tarch::multicore::Lock lock(_mapAccessSemaphore);
  assertion( _map.count(number)==1 );
  assertion( _map[number].locked );
  assertionEquals( _map[number].rankThatLastLocked, forRank );
  _map[number].locked = false;

  logDebug( "unlockSemaphoreOnGlobalMaster()", "successfully released lock " << number << ". state=" << toString() );
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::serveLockRequests() {
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());

  #ifdef Parallel
  tarch::multicore::Lock lock(_reserverationRequestsSemaphore);

  bool servedLockRequest = true;
  while (servedLockRequest) {
    servedLockRequest = false;
    {
      for (auto request = _pendingLockRequests.begin(); request != _pendingLockRequests.end(); ) {
        if ( tryLockSemaphoreOnGlobalMaster(request->second, request->first) ) {
          servedLockRequest = true;
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
        tarch::multicore::Lock lock(_reserverationRequestsSemaphore);
        std::pair<int,int> newEntry( status.MPI_SOURCE, number );
        _pendingLockRequests.push_back(newEntry);
        logDebug( "receiveDanglingMessages()", "there are " << _pendingLockRequests.size() << " lock requests in total. state=" << toString() );
      }
      else {
        releaseLock(-number);
      }
    }
    else {
      serveLockRequests();
    }
    #endif
  }
}


int tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getNumberOfLockedSemaphores() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    tarch::multicore::Lock lock(_reserverationRequestsSemaphore);
    return _pendingLockRequests.size();
  } else {
    return 0;
  }
}


tarch::mpi::BooleanSemaphore::BooleanSemaphoreService& tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance() {
  return _singleton;
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::addMapEntryLazily(int number) {
  assertion1( number>0, number );
  if ( _map.count(number)==0 ) {
    _map.insert( std::pair<int,SemaphoreMapEntry>(number,SemaphoreMapEntry()) );
  }
}


void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::acquireLock( int number ) {
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    lockSemaphoreOnGlobalMaster(number, tarch::mpi::Rank::getGlobalMasterRank());
  }
  else {
    #ifdef Parallel
    logDebug( "acquireLock()", "have to acquire lock on global master " << tarch::mpi::Rank::getGlobalMasterRank() << " and thus send master a " << number );
    
    MPI_Send( &number, 1, MPI_INT, tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator() );

    logDebug( "acquireLock()", "wait for confirmation from global master rank" );
    MPI_Request request;
    tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
    tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();

    MPI_Irecv( &number, 1, MPI_INT, tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag, tarch::mpi::Rank::getInstance().getCommunicator(), &request );
    int flag = 0;
    while ( not flag ) {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::", "acquireLock(int)", tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::", "acquireLock(int)", tarch::mpi::Rank::getGlobalMasterRank(), _semaphoreTag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();

      // See documentation on dangling messages
      //receiveDanglingMessages();
      MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
    }
    #else
    assertionMsg( false, "may not happen" );
    #endif
  }
}

void tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::releaseLock( int number ) {
  assertion( number>0 );
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    unlockSemaphoreOnGlobalMaster(number, tarch::mpi::Rank::getGlobalMasterRank());
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
