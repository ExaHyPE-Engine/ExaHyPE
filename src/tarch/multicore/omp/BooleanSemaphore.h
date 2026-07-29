// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"
#if !defined(_TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_) && defined(PEANO_WITH_OPENMP)
#define _TARCH_MULTICORE_BOOLEAN_SEMAPHORE_OMP_H_

#include "tarch/multicore/BooleanSemaphore.h"

#include <omp.h>

class tarch::multicore::BooleanSemaphore {
public:
  BooleanSemaphore();
  ~BooleanSemaphore();

  using Lock = tarch::multicore::Lock;

  /**
   * A semaphore owns its lock, so copying or moving one would leave two
   * objects guarding the same critical section.
   */
  BooleanSemaphore(const BooleanSemaphore&)            = delete;
  BooleanSemaphore(BooleanSemaphore&&)                 = delete;
  BooleanSemaphore& operator=(const BooleanSemaphore&) = delete;
  BooleanSemaphore& operator=(BooleanSemaphore&&)      = delete;

private:
  friend class tarch::multicore::Lock;
  friend class MultiReadSingleWriteSemaphore;

  omp_lock_t _lock;

  void enterCriticalSection();
  void leaveCriticalSection();
  bool tryEnterCriticalSection();
};

#endif
