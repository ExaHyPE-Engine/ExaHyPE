#include "../multicore.h"


#ifdef SharedOMP
#include "RecursiveSemaphore.h"

tarch::multicore::RecursiveSemaphore::RecursiveSemaphore() {
  omp_init_nest_lock(&lock);
}


tarch::multicore::RecursiveSemaphore::~RecursiveSemaphore() {
}


void tarch::multicore::RecursiveSemaphore::enterCriticalSection() {
  omp_set_nest_lock(&lock);
}


void tarch::multicore::RecursiveSemaphore::leaveCriticalSection() {
  omp_unset_nest_lock(&lock);
}


bool tarch::multicore::RecursiveSemaphore::tryEnterCriticalSection() {
  return omp_test_nest_lock(&lock);
}


#endif

