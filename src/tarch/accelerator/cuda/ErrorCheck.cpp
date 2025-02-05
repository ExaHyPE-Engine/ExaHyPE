// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorCheck.h"

#if defined(GPUOffloadingCUDA)

#include "tarch/Assertions.h"

#include <sstream>

void tarch::accelerator::cuda::checkError(cudaError_t code, const char* file, int line) {
  assertion(code == cudaSuccess);
  if (code != cudaSuccess) {
    cudaDeviceSynchronize();
    cudaDeviceReset();
    std::stringstream ss;
    ss << "CUDA assertion: " << cudaGetErrorString(code) << " " << file << " "
       << line;
    throw std::runtime_error(ss.str());
  }
}

void tarch::accelerator::cuda::checkLastError(const char* file, int line) {
  cudaError_t code = cudaGetLastError();
  assertion(code == cudaSuccess);
  if (code != cudaSuccess) {
    cudaDeviceSynchronize();
    cudaDeviceReset();
    std::stringstream ss;
    ss << "CUDA assertion: " << cudaGetErrorString(code) << " " << file << " "
       << line;
    throw std::runtime_error(ss.str());
  }
}

#endif
