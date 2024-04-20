#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"


tarch::multicore::Lock::Lock( tarch::multicore::BooleanSemaphore& semaphore, bool aquireLockImmediately, bool freeLockInDestructor ):
  _semaphore(semaphore),
  _lockIsAquired(false),
  _freeLockInDestructor(freeLockInDestructor) {
  if (aquireLockImmediately) {
    lock();
  }
}


tarch::multicore::Lock::~Lock() {
  if (_lockIsAquired and _freeLockInDestructor) {
    free();
  }
}


bool tarch::multicore::Lock::tryLock() {
  assertion( !_lockIsAquired );
  _lockIsAquired = _semaphore.tryEnterCriticalSection();
  return _lockIsAquired;
}


void tarch::multicore::Lock::lock() {
  assertion( !_lockIsAquired );
  _semaphore.enterCriticalSection();
  _lockIsAquired = true;
}


void tarch::multicore::Lock::free() {
  assertion( _lockIsAquired or not _freeLockInDestructor );
  _semaphore.leaveCriticalSection();
  _lockIsAquired = false;
}

