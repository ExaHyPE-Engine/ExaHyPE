// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "Device.h"

namespace tarch::accelerator {
  inline __host__ __device__ void abort(const char* message) {
    COMPILE_IF_ON_DEVICE(
      const char empty[] = "";
      __assert_fail(message, empty, 0, empty);
      while (true) {
      }
    )
    COMPILE_IF_ON_HOST(
      std::fputs(message, stderr);
      std::fflush(stderr);
      std::_Exit(EXIT_FAILURE);
    )
  }
} // namespace tarch::accelerator

#endif
