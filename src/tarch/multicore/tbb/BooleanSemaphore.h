// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include "tarch/multicore/multicore.h"

namespace tarch {
  namespace multicore {
    class BooleanSemaphore;
    class Lock;
  }
}

#if defined(SharedTBB)

#include <tbb/spin_mutex.h>

class tarch::multicore::BooleanSemaphore {
  private:
    tbb::spin_mutex   _mutex;

    friend class tarch::multicore::Lock;
    friend class RecursiveSemaphore;

    void enterCriticalSection();
    void leaveCriticalSection();
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    inline BooleanSemaphore([[maybe_unused]] const BooleanSemaphore& semaphore) {}

    /**
     * You may not copy a semaphore
     */
    inline BooleanSemaphore& operator = ([[maybe_unused]] const BooleanSemaphore& semaphore) { return *this; }

  public:
    BooleanSemaphore();
    ~BooleanSemaphore();
};

#endif
