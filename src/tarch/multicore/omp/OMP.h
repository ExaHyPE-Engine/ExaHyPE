// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"


#if !defined(_TARCH_MULTICORE_OMP_OMP_H_) && defined(SharedOMP)
#define _TARCH_MULTICORE_OMP_OMP_H_

namespace tarch {
  namespace multicore {
    /**
     * Configure OMP runtime.
     */
    extern int MaxNumberOfConcurrentHighBandwidthTasks;
    extern int MaxNumberOfConcurrentBackgroundTasks;
    namespace omp {
      /**
       * I implement this guy in Tasks.cpp
       */
      void shutdownConsumerTasks();
    }
  }
}

#endif
