// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if !defined(_TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_) && defined(SharedTBB)
#define _TARCH_MULTICORE_RECURSIVE_SEMAPHORE_H_


//#include <tbb/recursive_mutex.h>
#include <mutex>


namespace tarch {
  namespace multicore {
    class RecursiveSemaphore;
    class RecursiveLock;
  }
}


/**
 * Recursive Semaphore
 *
 * A recursive semaphore is a boolean semaphore that one thread (the first one)
 * can lock an arbitrary number of times.
 *
 * Using semaphores in libraries is very tricky. This one is mainly used by
 * the services. See tarch::services::Service::receiveDanglingMessagesSemaphore
 * for some remarks.
 *
 * <h2> C++17 vs. new TBB versions </h2>
 *
 * New TBB versions have deprecated the tbb::recursive_mutex class in favour of
 * the C++ version. I have switched to the C++ version but the TBB version
 * remains in the code (commented out).
 *
 * @author Tobias Weinzierl
 */
class tarch::multicore::RecursiveSemaphore {
  private:
    friend class tarch::multicore::RecursiveLock;

    //tbb::recursive_mutex          _recursiveMutex;
    std::recursive_mutex          _recursiveMutex;
    //std::recursive_timed_mutex    _recursiveMutex;

    void enterCriticalSection();
    void leaveCriticalSection();
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
