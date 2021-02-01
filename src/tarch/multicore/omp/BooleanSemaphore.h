// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#if !defined( _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_) && defined(SharedOMP)
#define _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_


namespace tarch {
  namespace multicore {
    class BooleanSemaphore;
    class Lock;
  }
}


#include <string>
#include <omp.h>
#include "omp.h"

class tarch::multicore::BooleanSemaphore {
  private:
    friend class tarch::multicore::Lock;
    omp_lock_t lock;

    void enterCriticalSection();
    void leaveCriticalSection();
    bool tryEnterCriticalSection();

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore( const BooleanSemaphore& semaphore ) {}

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore& operator=( const BooleanSemaphore& semaphore ) {return *this;}
  public:
    BooleanSemaphore();
    ~BooleanSemaphore();
};
#endif
