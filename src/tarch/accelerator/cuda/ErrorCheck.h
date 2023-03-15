// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(__NVCOMPILER_CUDA__)
#include <cuda_runtime.h>

namespace tarch::accelerator::cuda {
  void checkError(cudaError_t code, const char *file, int line);
}

#define CHECK_CUDA_ERROR(ans) { tarch::accelerator::cuda::checkError((ans), __FILE__, __LINE__); }

#else
#define CHECK_CUDA_ERROR(ans)
#endif
