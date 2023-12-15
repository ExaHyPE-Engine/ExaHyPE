
#include "config.h"

#if defined(GPUOffloadingCUDA)

#include <iostream>
#include <stdio.h>
#include <vector>

__global__ void testKernelBody(int* a) { (*a)++; }

std::vector<bool> testKernelLauncher() {
  int numberOfDevices;
  cudaGetDeviceCount(&numberOfDevices);

  std::vector<bool> offloadResults(numberOfDevices, false);
  int               savedCurrentDevice{0};

  cudaGetDevice(&savedCurrentDevice);

  for (int i = 0; i < numberOfDevices; i++) {
    cudaSetDevice(i);
    int* dev_counter;
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

// Avoid templates so we do need to include the functions to .cpp files we have
__global__ void copyKernelDouble(const double* a, double* b, size_t N) {
  const int i = blockDim.x * blockIdx.x + threadIdx.x;
  b[i] = a[i];
}

void maxPotentialBlockSize(int* minGridSize, int* blockSize) {
  cudaOccupancyMaxPotentialBlockSize(minGridSize, blockSize, copyKernelDouble, 0, 0);
}

void copyKernelLauncher(const double* a, double* b, size_t N, int gridSize, int blockSize) {
  copyKernelDouble<<<gridSize, blockSize, 0>>>(a, b, N);
}

#else

#error "GPUOffloadingCUDA is not defined"

#endif