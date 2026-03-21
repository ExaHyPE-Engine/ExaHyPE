// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_HIP)

#include <hip/hip_runtime.h>

namespace tarch::accelerator::hip {
  void checkError(hipError_t code, const char* expr, const char* file, int line);
  void checkLastError(const char* file, int line);
} // namespace tarch::accelerator::hip

#define CHECK_HIP_ERROR(ans) \
  do { \
    tarch::accelerator::hip::checkError((ans), #ans, __FILE__, __LINE__); \
  } while (0)
#if PEANO_DEBUG > 0
#define CHECK_HIP_LAST_ERROR() \
  { \
    tarch::accelerator::hip::checkLastError(__FILE__, __LINE__); \
  }
#else
#define CHECK_HIP_LAST_ERROR()
#endif

#endif // defined(WITH_GPU_HIP)
