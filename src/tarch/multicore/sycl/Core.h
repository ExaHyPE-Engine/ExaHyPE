// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"


/**
 * @see tarch_accelerator_SYCL in Device.h
 */
#if defined(SharedSYCL) and !defined(_TARCH_MULTICORE_SYCL_CORE_H_)
#define _TARCH_MULTICORE_SYCL_CORE_H_

#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")

namespace tarch {
  namespace multicore {
    /**
     * @return SYCL queue for the host
     */
    sycl::queue& getHostSYCLQueue();
  }
}

#endif
