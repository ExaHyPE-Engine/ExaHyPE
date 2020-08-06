// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"


#if !defined(_TARCH_MULTICORE_TBB_TBB_H_) && defined(SharedTBB)
#define _TARCH_MULTICORE_TBB_TBB_H_

namespace tarch {
  namespace multicore {
    /**
     * Configure TBB runtime.
     */
    extern int MaxNumberOfConcurrentHighBandwidthTasks;
    extern int MaxNumberOfConcurrentBackgroundTasks;
    namespace tbb {
      /**
       * I implement this guy in Tasks.cpp
       */
      void shutdownTaskEnvironment();
    }
  }
}

#endif
