// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_CUDA)

#include <cuda_runtime.h>

namespace tarch::accelerator::cuda {
  void checkError(cudaError_t code, const char* expr, const char* file, int line);
  void checkLastError(const char* file, int line);
} // namespace tarch::accelerator::cuda

#define CHECK_CUDA_ERROR(ans) do { \
  tarch::accelerator::cuda::checkError((ans), #ans, __FILE__, __LINE__); \
} while (0)
#if PEANO_DEBUG > 0
#define CHECK_CUDA_LAST_ERROR() { tarch::accelerator::cuda::checkLastError(__FILE__, __LINE__); }
#else
#define CHECK_CUDA_LAST_ERROR()
#endif

#endif // defined(WITH_GPU_CUDA)
