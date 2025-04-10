// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"

#if defined(SharedSYCL)

namespace tarch {
  namespace multicore {
    /**
     * @return SYCL queue for the host
     */
    void* getHostSYCLQueue();
  }
}

#endif // defined(SharedSYCL)
