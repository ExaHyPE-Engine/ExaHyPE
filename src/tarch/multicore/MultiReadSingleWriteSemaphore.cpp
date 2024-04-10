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
  volatile bool letWriteAccessOvertake = _highPriorityToWrites;
  while (letWriteAccessOvertake) {
    _semaphore.enterCriticalSection();
    letWriteAccessOvertake = _numberOfPendingWriteRequests>0
                         and _numberOfActiveReadAccesses==0;
    _semaphore.leaveCriticalSection();
  }

  volatile bool success = false;
  while (not success) {
    _semaphore.enterCriticalSection();
    success = not _writeAccessAcquired;
    if (success) {
      // point out to compiler that we update, as volatile ++ is deprecated
      int updatedNumberOfActiveReadAccesses = _numberOfActiveReadAccesses;
      updatedNumberOfActiveReadAccesses++;
      _numberOfActiveReadAccesses = updatedNumberOfActiveReadAccesses;
    }
    _semaphore.leaveCriticalSection();
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalWriteSection() {
  if (_highPriorityToWrites) {
    _semaphore.enterCriticalSection();
    // point out to compiler that we update, as volatile ++ is deprecated
    int updatedNuberOfPendingWriteRequests = _numberOfPendingWriteRequests;
    updatedNuberOfPendingWriteRequests++;
    _numberOfPendingWriteRequests = updatedNuberOfPendingWriteRequests;
    _semaphore.leaveCriticalSection();
  }

  volatile bool success = false;

  while (not success) {
    _semaphore.enterCriticalSection();
    success = not _writeAccessAcquired and (_numberOfActiveReadAccesses==0);
    if (success) {
      if (_highPriorityToWrites) {
        // point out to compiler that we update, as volatile ++ is deprecated
        int updatedNuberOfPendingWriteRequests = _numberOfPendingWriteRequests;
        updatedNuberOfPendingWriteRequests--;
        _numberOfPendingWriteRequests = updatedNuberOfPendingWriteRequests;
      }
      _writeAccessAcquired = true;
    }
    _semaphore.leaveCriticalSection();
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalReadSection() {
  _semaphore.enterCriticalSection();
  // point out to compiler that we update, as volatile ++ is deprecated
  int updatedNumberOfActiveReadAccesses = _numberOfActiveReadAccesses;
  updatedNumberOfActiveReadAccesses--;
  _numberOfActiveReadAccesses = updatedNumberOfActiveReadAccesses;
  assertion( _numberOfActiveReadAccesses>=0 );
  _semaphore.leaveCriticalSection();
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalWriteSection() {
  _semaphore.enterCriticalSection();
  assertion(_writeAccessAcquired);
  _writeAccessAcquired = false;
  _semaphore.leaveCriticalSection();
}
