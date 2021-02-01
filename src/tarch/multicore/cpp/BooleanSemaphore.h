// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_CPP_BOOLEAN_SEMAPHORE_H_
#define _TARCH_MULTICORE_CPP_BOOLEAN_SEMAPHORE_H_

#include <string>
#include "tarch/multicore/multicore.h"


namespace tarch {
  namespace multicore {
    class BooleanSemaphore;
    class Lock;
  }
}


#if defined(SharedCPP)
#include <mutex>


class tarch::multicore::BooleanSemaphore {
  private:
	std::mutex   _mutex;

    friend class tarch::multicore::Lock;
    friend class RecursiveSemaphore;

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


#endif
