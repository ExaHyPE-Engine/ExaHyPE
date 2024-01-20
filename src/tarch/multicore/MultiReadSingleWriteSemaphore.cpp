#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/multicore/Lock.h"
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
  volatile bool success = false;

  while (not success) {
    tarch::multicore::Lock lock(_semaphore);
    success = not _writeAccessAcquired and _numberOfPendingWriteRequests==0;
    if (success) {
      _numberOfActiveReadAccesses++;
    }
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalWriteSection() {
  if (_highPriorityToWrites) {
    tarch::multicore::Lock lock(_semaphore);
    _numberOfPendingWriteRequests++;
  }

  volatile bool success = false;

  while (not success) {
    tarch::multicore::Lock lock(_semaphore);
    success = not _writeAccessAcquired and (_numberOfActiveReadAccesses==0);
    if (success) {
      if (_highPriorityToWrites) {
        _numberOfPendingWriteRequests--;
      }
      _writeAccessAcquired = true;
    }
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalReadSection() {
  tarch::multicore::Lock lock(_semaphore);
  _numberOfActiveReadAccesses--;
  assertion( _numberOfActiveReadAccesses>=0 );
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalWriteSection() {
  tarch::multicore::Lock lock(_semaphore);
  assertion(_writeAccessAcquired);
  _writeAccessAcquired = false;
}
