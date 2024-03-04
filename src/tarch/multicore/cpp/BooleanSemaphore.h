// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"


#if defined(SharedCPP)
#include <mutex>

class tarch::multicore::BooleanSemaphore {
  private:
    std::mutex _mutex;

    friend class tarch::multicore::Lock;
    friend class RecursiveSemaphore;
    friend class tarch::mpi::BooleanSemaphore;
    friend class MultiReadSingleWriteSemaphore;

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
    BooleanSemaphore& operator = (const BooleanSemaphore&) { return *this; }

  public:
    BooleanSemaphore();
    ~BooleanSemaphore();
};

#endif
