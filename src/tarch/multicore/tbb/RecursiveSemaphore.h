// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "config.h"

#include "tarch/multicore/multicore.h"

#if defined(SharedTBB)

#include <string>
#include <thread>

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"

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
 * A recursive semaphore is a boolean semphore that one thread (the first one)
 * can lock an arbitrary number of times.
 *
 * @author Tobias Weinzierl
 */
class tarch::multicore::RecursiveSemaphore {
  private:
    friend class tarch::multicore::RecursiveLock;

    std::recursive_mutex   _mutex;

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
    RecursiveSemaphore& operator = (const RecursiveSemaphore&) { return *this; }

  public:
    RecursiveSemaphore();
    ~RecursiveSemaphore();
};

#endif
