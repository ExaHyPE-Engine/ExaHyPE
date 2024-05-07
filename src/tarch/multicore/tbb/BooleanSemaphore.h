// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"


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
    friend class tarch::multicore::Lock;
    friend class tarch::mpi::BooleanSemaphore;
    friend class MultiReadSingleWriteSemaphore;
    friend class RecursiveSemaphore;

    tbb::spin_mutex   _mutex;

    void enterCriticalSection();
    void leaveCriticalSection();
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    inline BooleanSemaphore(const BooleanSemaphore&) {}

    /**
     * You may not copy a semaphore
     */
    inline BooleanSemaphore& operator = (const BooleanSemaphore&) { return *this; }

  public:
    BooleanSemaphore();
    ~BooleanSemaphore();
};

#endif
