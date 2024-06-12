
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "config.h"

#if defined(GPUOffloadingCUDA)

#include <iostream>
#include <stdio.h>
#include <vector>

namespace tarch::accelerator::cuda {
  __global__ void testKernelBody(int* a) { (*a)++; }

  std::vector<bool> testKernelLauncher() {
    int numberOfDevices = 0;
    cudaGetDeviceCount(&numberOfDevices);

    std::vector<bool> offloadResults(numberOfDevices, false);
    int               savedCurrentDevice{0};

    cudaGetDevice(&savedCurrentDevice);

    for (int i = 0; i < numberOfDevices; i++) {
      cudaSetDevice(i);
      int* dev_counter = nullptr;
      cudaMalloc(&dev_counter, sizeof(int));
      cudaMemset(dev_counter, i, sizeof(int));
      testKernelBody<<<32, 32>>>(dev_counter);
      cudaDeviceSynchronize();
      cudaError_t err   = cudaGetLastError();
      offloadResults[i] = err == cudaSuccess;
      cudaDeviceReset();
    }

    cudaSetDevice(savedCurrentDevice);

    return offloadResults;
  }

  __global__ void copyKernelDouble(const double* a, double* b, size_t N) {
    const int i = blockDim.x * blockIdx.x + threadIdx.x;
    b[i]        = a[i];
  }

  void maxPotentialBlockSize(int* minGridSize, int* blockSize) {
    cudaOccupancyMaxPotentialBlockSize(
      minGridSize,
      blockSize,
      copyKernelDouble,
      0,
      0
    );
  }

  void copyKernelLauncher(
    const double* a,
    double*       b,
    size_t        N,
    int           gridSize,
    int           blockSize
  ) {
    copyKernelDouble<<<gridSize, blockSize, 0>>>(a, b, N);
  }
} // namespace tarch::accelerator::cuda

#else

#error "GPUOffloadingCUDA is not defined"

#endif
