#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/Assertions.h"


#if defined(SharedTBB) or defined(SharedTBBExtension)

tarch::multicore::MultiReadSingleWriteSemaphore::MultiReadSingleWriteSemaphore(bool highPriorityToWrites):
  _mutex() {
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalReadSection() {
  _mutex.lock_shared();
}


void tarch::multicore::MultiReadSingleWriteSemaphore::enterCriticalWriteSection() {
  _mutex.lock();
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalReadSection() {
  _mutex.unlock_shared();
}


void tarch::multicore::MultiReadSingleWriteSemaphore::leaveCriticalWriteSection() {
  _mutex.unlock();
}

#endif

