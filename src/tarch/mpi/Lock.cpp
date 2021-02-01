#include "tarch/Assertions.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/BooleanSemaphore.h"


tarch::mpi::Lock::Lock( tarch::mpi::BooleanSemaphore& semaphore, bool aquireLockImmediately ):
  _semaphore(semaphore),
  _lockIsAquired(false) {
  if (aquireLockImmediately) {
    lock();
  }
}


tarch::mpi::Lock::~Lock() {
  if (_lockIsAquired) {
    free();
  }
}


void tarch::mpi::Lock::lock() {
  assertion( !_lockIsAquired );
  _semaphore.enterCriticalSection();
  _lockIsAquired = true;
}


void tarch::mpi::Lock::free() {
  assertion( _lockIsAquired );
  _semaphore.leaveCriticalSection();
  _lockIsAquired = false;
}

