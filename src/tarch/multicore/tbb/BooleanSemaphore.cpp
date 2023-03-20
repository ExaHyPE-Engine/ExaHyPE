#include "BooleanSemaphore.h"

#if defined(SharedTBB)

tarch::multicore::BooleanSemaphore::BooleanSemaphore() {}

tarch::multicore::BooleanSemaphore::~BooleanSemaphore() {}

void tarch::multicore::BooleanSemaphore::enterCriticalSection() {
  _mutex.lock();
}

void tarch::multicore::BooleanSemaphore::leaveCriticalSection() {
  _mutex.unlock();
}

bool tarch::multicore::BooleanSemaphore::tryEnterCriticalSection() {
  return _mutex.try_lock();
}

#endif
