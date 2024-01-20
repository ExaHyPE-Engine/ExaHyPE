#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/Assertions.h"


tarch::multicore::MultiReadSingleWriteSemaphore::MultiReadSingleWriteSemaphore(bool highPriorityToWrites):
  _highPriorityToWrites(highPriorityToWrites),
  _writeAccessAcquired(false),
  _numberOfPendingWriteRequests(0),
  _numberOfActiveReadAccesses(0) {}


tarch::multicore::MultiReadSingleWriteSemaphore::~MultiReadSingleWriteSemaphore() {
  assertion(not _writeAccessAcquired );
  assertionEquals( _numberOfActiveReadAccesses, 0 );
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalReadSection() {
  volatile bool letWriteAccessOvertake = not _highPriorityToWrites;
  if (letWriteAccessOvertake) {
    if (_semaphore.tryEnterCriticalSection()) {
      letWriteAccessOvertake = _numberOfPendingWriteRequests>0;
      _semaphore.leaveCriticalSection();
    }
  }

  volatile bool success = false;
  while (not success) {
    if (_semaphore.tryEnterCriticalSection()) {
      success = not _writeAccessAcquired;
      if (success) {
        _numberOfActiveReadAccesses++;
      }
      _semaphore.leaveCriticalSection();
    }
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalWriteSection() {
  if (_highPriorityToWrites) {
    _semaphore.enterCriticalSection();
    _numberOfPendingWriteRequests++;
    _semaphore.leaveCriticalSection();
  }

  volatile bool success = false;

  while (not success) {
    if (_semaphore.tryEnterCriticalSection()) {
      success = not _writeAccessAcquired and (_numberOfActiveReadAccesses==0);
      if (success) {
        if (_highPriorityToWrites) {
          _numberOfPendingWriteRequests--;
        }
        _writeAccessAcquired = true;
      }
      _semaphore.leaveCriticalSection();
    }
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalReadSection() {
  _semaphore.enterCriticalSection();
  _numberOfActiveReadAccesses--;
  assertion( _numberOfActiveReadAccesses>=0 );
  _semaphore.leaveCriticalSection();
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalWriteSection() {
  _semaphore.enterCriticalSection();
  assertion(_writeAccessAcquired);
  _writeAccessAcquired = false;
  _semaphore.leaveCriticalSection();
}
