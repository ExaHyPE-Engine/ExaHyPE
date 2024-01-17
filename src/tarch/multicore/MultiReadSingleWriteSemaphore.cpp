#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/Assertions.h"


tarch::multicore::MultiReadSingleWriteSemaphore::MultiReadSingleWriteSemaphore():
  _writeAccessRequired(false),
  _numberOfActiveReadAccesses(0) {}


tarch::multicore::MultiReadSingleWriteSemaphore::~MultiReadSingleWriteSemaphore() {
  assertion(not _writeAccessRequired );
  assertionEquals( _numberOfActiveReadAccesses, 0 );
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalReadSection() {
  volatile bool success = false;

  while (not success) {
    tarch::multicore::Lock lock(_semaphore);
    success = not _writeAccessRequired;
    if (success) {
      _numberOfActiveReadAccesses++;
    }
  }
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalWriteSection() {
  volatile bool success = false;

  while (not success) {
    tarch::multicore::Lock lock(_semaphore);
    success = not _writeAccessRequired and (_numberOfActiveReadAccesses==0);
    if (success) {
      _writeAccessRequired = true;
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
  assertion(_writeAccessRequired);
  _writeAccessRequired = false;
}
