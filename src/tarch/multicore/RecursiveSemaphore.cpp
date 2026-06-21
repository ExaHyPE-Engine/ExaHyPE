#include "tarch/multicore/RecursiveSemaphore.h"


#ifndef PEANO_WITH_MULTITHREADING
tarch::multicore::RecursiveSemaphore::RecursiveSemaphore() {
}


tarch::multicore::RecursiveSemaphore::~RecursiveSemaphore() {
}


void tarch::multicore::RecursiveSemaphore::enterCriticalSection() {
}


void tarch::multicore::RecursiveSemaphore::leaveCriticalSection() {
}


bool tarch::multicore::RecursiveSemaphore::tryEnterCriticalSection() {
  return true;
}
#endif
