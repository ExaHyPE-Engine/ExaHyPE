// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"
#if !defined( _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_) && defined(SharedOMP)
#define _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_

#include "tarch/multicore/BooleanSemaphore.h"
#include <string>
#include <omp.h>

class tarch::multicore::BooleanSemaphore {
  private:
    friend class tarch::multicore::Lock;
    friend class tarch::mpi::BooleanSemaphore;
    friend class MultiReadSingleWriteSemaphore;
    friend class RecursiveSemaphore;

    omp_lock_t lock;

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
