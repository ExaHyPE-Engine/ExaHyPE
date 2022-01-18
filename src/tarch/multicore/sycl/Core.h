// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_SYCL_CORE_H_
#define _TARCH_MULTICORE_SYCL_CORE_H_

#include <string>
#include "tarch/multicore/multicore.h"


#if defined(SharedSYCL)

namespace tarch {
  namespace multicore {
    sycl::queue& getSYCLQueue( int number );
  }
}


#endif
#endif
