// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"

#if defined(GPUOffloadingCUDA) or defined(__NVCOMPILER_CUDA__) \
  or defined(__CUDACC__) or defined(NVCC) or defined(_NVHPC_CUDA)

#include <cuda_runtime.h>

namespace tarch::accelerator::cuda {
  void checkError(cudaError_t code, const char* file, int line);
  void checkLastError(const char* file, int line);
} // namespace tarch::accelerator::cuda

#define CHECK_CUDA_ERROR(ans) \
  { tarch::accelerator::cuda::checkError((ans), __FILE__, __LINE__); }
#define CHECK_CUDA_LAST_ERROR() \
  { tarch::accelerator::cuda::checkLastError(__FILE__, __LINE__); }

#else

#define CHECK_CUDA_ERROR(ans)
#define CHECK_CUDA_LAST_ERROR()

#endif
