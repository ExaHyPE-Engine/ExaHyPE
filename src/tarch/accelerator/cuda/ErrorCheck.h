// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include <cuda_runtime.h>

namespace tarch::accelerator::cuda {
  void checkError(cudaError_t code, const char* expr, const char* file, int line);
} // namespace tarch::accelerator::cuda

#define CHECK_CUDA_ERROR(ans) \
  do { \
    tarch::accelerator::cuda::checkError((ans), #ans, __FILE__, __LINE__); \
  } while (0)

#endif // defined(PEANO_WITH_GPU_BACKEND_CUDA)
