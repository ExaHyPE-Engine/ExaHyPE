#include "tarch/multicore/multicore.h"


#if defined(SharedOMP)
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/logging/Log.h"


#include <limits>


tarch::multicore::BooleanSemaphore::BooleanSemaphore() {
  omp_init_lock(&lock);
}


tarch::multicore::BooleanSemaphore::~BooleanSemaphore() {
}


void tarch::multicore::BooleanSemaphore::enterCriticalSection() {
  omp_set_lock(&lock);
}


void tarch::multicore::BooleanSemaphore::leaveCriticalSection() {
  omp_unset_lock(&lock);
}

bool tarch::multicore::BooleanSemaphore::tryEnterCriticalSection() {
  return omp_test_lock(&lock);
}
#endif
