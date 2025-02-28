// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorCheck.h"

#if defined(GPUOffloadingCUDA)

#include "tarch/Assertions.h"

#include <sstream>

void tarch::accelerator::cuda::checkError(cudaError_t code, const char* expr, const char* file, int line) {
  if (code != cudaSuccess) {
    cudaDeviceSynchronize();
    cudaDeviceReset();
    std::stringstream ss;
    ss << "CUDA assertion in " << expr << " " << file << "(" << line << "): " << cudaGetErrorString(code);
    throw std::runtime_error(ss.str());
  }
}

void tarch::accelerator::cuda::checkLastError(const char* file, int line) {
  cudaError_t code = cudaGetLastError();
  if (code != cudaSuccess) {
    cudaDeviceSynchronize();
    cudaDeviceReset();
    std::stringstream ss;
    ss << "CUDA assertion in " << file << "(" << line << "):" << cudaGetErrorString(code);
    throw std::runtime_error(ss.str());
  }
}

#endif
