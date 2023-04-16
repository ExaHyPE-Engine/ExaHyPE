#include "tarch/Assertions.h"
#include "tarch/multicore/RecursiveLock.h"
#include "tarch/multicore/RecursiveSemaphore.h"


tarch::multicore::RecursiveLock::RecursiveLock( tarch::multicore::RecursiveSemaphore& semaphore, bool aquireLockImmediately ):
  _semaphore(semaphore),
  _lockIsAquired(false) {
  if (aquireLockImmediately) {
    lock();
  }
}


tarch::multicore::RecursiveLock::~RecursiveLock() {
  if (_lockIsAquired) {
    free();
  }
}


bool tarch::multicore::RecursiveLock::tryLock() {
  assertion( !_lockIsAquired );
  bool result = _semaphore.tryEnterCriticalSection();
  if (result) {
    _lockIsAquired = true;
  }
  return result;
}


bool tarch::multicore::RecursiveLock::isLocked() const {
  return _lockIsAquired;
}


void tarch::multicore::RecursiveLock::lock() {
  assertion( !_lockIsAquired );
  _semaphore.enterCriticalSection();
  _lockIsAquired = true;
}


void tarch::multicore::RecursiveLock::free() {
  assertion( _lockIsAquired );
  _semaphore.leaveCriticalSection();
  _lockIsAquired = false;
}

