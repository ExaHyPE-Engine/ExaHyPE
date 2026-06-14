// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_CUDA)

#include "Device.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

extern "C" [[noreturn]] __device__ void __assertfail(
  const void* message,
  const void* file,
  unsigned int line,
  const void* function,
  std::size_t charSize
);

namespace tarch::accelerator {
  inline __host__ __device__ void abort(const char* message) {
    COMPILE_IF_ON_DEVICE(
      const char empty[] = "";
      __assertfail(
        static_cast<const void*>(message),
        static_cast<const void*>(empty),
        0,
        static_cast<const void*>(empty),
        sizeof(char)
      );
    )
    COMPILE_IF_ON_HOST(
      std::fputs(message, stderr);
      std::fflush(stderr);
      std::_Exit(EXIT_FAILURE);
    )
  }
} // namespace tarch::accelerator

#endif
