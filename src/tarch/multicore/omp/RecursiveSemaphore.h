// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"
#if !defined(_TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_) && defined(SharedOMP)
#define _TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_

namespace tarch {
  namespace multicore {
    class RecursiveSemaphore;
    class RecursiveLock;
  }
}

#include <string>
#include <thread>
#include "omp.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"

/**
 * Recursive Semaphore
 *
 * A recursive semaphore is a boolean semphore that one thread (the first one)
 * can lock an arbitrary number of times.
 *
 *
 * @author Tobias Weinzierl
 */
class tarch::multicore::RecursiveSemaphore {
  private:
    friend class tarch::multicore::RecursiveLock;

    omp_nest_lock_t lock;

    void enterCriticalSection();
    void leaveCriticalSection();
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    inline RecursiveSemaphore(const RecursiveSemaphore&) {}

    /**
     * You may not copy a semaphore
     */
    inline RecursiveSemaphore& operator = (const RecursiveSemaphore&) { return *this; }

  public:
    RecursiveSemaphore();
    ~RecursiveSemaphore();
};

#endif
