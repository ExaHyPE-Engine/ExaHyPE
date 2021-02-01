#include "tarch/multicore/multicore.h"


#if !defined(SharedMemoryParallelisation)
#include "tarch/multicore/BooleanSemaphore.h"
tarch::multicore::BooleanSemaphore::BooleanSemaphore() {
}


tarch::multicore::BooleanSemaphore::~BooleanSemaphore() {
}


void tarch::multicore::BooleanSemaphore::enterCriticalSection() {
}


void tarch::multicore::BooleanSemaphore::leaveCriticalSection() {
}


bool tarch::multicore::BooleanSemaphore::tryEnterCriticalSection() {
  return true;
}

#endif
