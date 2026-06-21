#include "tarch/multicore/multicore.h"


#if !defined(PEANO_WITH_MULTITHREADING)
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
