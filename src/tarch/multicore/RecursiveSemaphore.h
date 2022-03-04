// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#if defined(SharedCPP)
  #include "cpp/RecursiveSemaphore.h"
#elif defined(SharedTBB)
  #include "tbb/RecursiveSemaphore.h"
#elif defined(SharedOMP)
  #include "omp/RecursiveSemaphore.h"
#elif !defined(_TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_)
#define _TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_

namespace tarch {
  namespace multicore {
    class RecursiveSemaphore;
    class RecursiveLock;
  }
}

#include <string>
#include <thread>

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

    /**
     * Waits until I can enter the critical section.
     */
    void enterCriticalSection();

    /**
     * Tells the semaphore that it is about to leave.
     */
    void leaveCriticalSection();

    /**
     * Run into critical section and try to lock. If we are successful,
     * the routine returns true and the stuff is locked (so please call
     * leave later on). Otherwise, I return false.
     */
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    RecursiveSemaphore( const RecursiveSemaphore& semaphore ) {}

    /**
     * You may not copy a semaphore
     */
    RecursiveSemaphore& operator=( const RecursiveSemaphore& semaphore ) {return *this;}
  public:
    RecursiveSemaphore();
    ~RecursiveSemaphore();
};


#endif
