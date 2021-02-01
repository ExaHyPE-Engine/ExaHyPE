// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"


#if !defined(_TARCH_MULTICORE_CPP_CPP_H_) && defined(SharedCPP)
#define _TARCH_MULTICORE_CPP_CPP_H_

namespace tarch {
  namespace multicore {
    namespace cpp {
      /**
       * I implement this guy in Tasks.cpp
       */
      void shutdownConsumerTasks();
    }
  }
}

#endif
