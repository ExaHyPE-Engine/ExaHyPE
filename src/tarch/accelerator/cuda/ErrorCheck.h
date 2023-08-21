// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#if defined(__NVCOMPILER_CUDA__) || defined(__CUDACC__) || defined(NVCC)
#include <sstream>
#include <cuda_runtime.h>

#include "tarch/Assertions.h"

namespace tarch::accelerator::cuda {
  inline void checkError(cudaError_t code, const char *file, int line) {
    assertion(code == cudaSuccess);
    if (code != cudaSuccess) {
      std::stringstream ss;
      ss << "CUDA assertion: " << cudaGetErrorString(code) << " " << file << " " << line;
      throw std::runtime_error(ss.str());
    }
  }
}


#define CHECK_CUDA_ERROR(ans) { tarch::accelerator::cuda::checkError((ans), __FILE__, __LINE__); }
#else
#define CHECK_CUDA_ERROR(ans)
#endif
