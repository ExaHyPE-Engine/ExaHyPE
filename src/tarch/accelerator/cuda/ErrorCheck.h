// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(__NVCOMPILER_CUDA__) || defined(__CUDACC__) || defined(NVCC)

namespace tarch::accelerator::cuda {
  void checkError(cudaError_t code, const char* file, int line);
  void checkLastError(const char* file, int line);
} // namespace tarch::accelerator::cuda

#define CHECK_CUDA_ERROR(ans) \
  { tarch::accelerator::cuda::checkError((ans), __FILE__, __LINE__); }
#define CHECK_CUDA_LAST_ERROR() \
  { tarch::accelerator::cuda::checkError(__FILE__, __LINE__); }

#else

#define CHECK_CUDA_ERROR(ans)
#define CHECK_CUDA_LAST_ERROR()

#endif
