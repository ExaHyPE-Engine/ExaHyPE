#include "tarch/Assertions.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"


tarch::multicore::MultiReadSingleWriteLock::MultiReadSingleWriteLock(
  tarch::multicore::MultiReadSingleWriteSemaphore& semaphore,
  bool isReadLock,
  bool aquireLockImmediately ):
  _semaphore(semaphore),
  _isReadLock(isReadLock),
  _lockIsAquired(false) {
  if (aquireLockImmediately) {
    lock();
  }
}


tarch::multicore::MultiReadSingleWriteLock::~MultiReadSingleWriteLock() {
  if (_lockIsAquired) {
    free();
  }
}


void tarch::multicore::MultiReadSingleWriteLock::lock() {
  assertion( !_lockIsAquired );
  if (_isReadLock) {
    _semaphore.enterCriticalReadSection();
  }
  else {
    _semaphore.enterCriticalWriteSection();
  }
  _lockIsAquired = true;
}


void tarch::multicore::MultiReadSingleWriteLock::free() {
  assertion( _lockIsAquired );
  if (_isReadLock) {
   _semaphore.leaveCriticalReadSection();
  }
  else {
    _semaphore.leaveCriticalWriteSection();
  }
  _lockIsAquired = false;
}

