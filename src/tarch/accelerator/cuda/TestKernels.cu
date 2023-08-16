
#if defined(GPUOffloadingCUDA)

#include <vector>
#include <stdio.h>
#include <iostream>

#include "DeviceInfo.h"

__global__ void testKernelBody(int *a)
{
  (*a)++;
}

std::vector<bool> testKernelLauncher()
{
  int numberOfDevices;
  cudaGetDeviceCount(&numberOfDevices);

  std::vector<bool> offloadResults(numberOfDevices, false);
  int savedCurrentDevice{0};

  cudaGetDevice(&savedCurrentDevice);

  for (int i = 0; i < numberOfDevices; i++)
  {
    cudaSetDevice(i);
    int *dev_counter;
    cudaMalloc(&dev_counter, sizeof(int));
    cudaMemset(dev_counter, i, sizeof(int));
    testKernelBody<<<32, 32>>>(dev_counter);
    cudaDeviceSynchronize();
    cudaError_t err = cudaGetLastError();
    offloadResults[i] = err == cudaSuccess;
    cudaDeviceReset();
  }

  cudaSetDevice(savedCurrentDevice);

  return offloadResults;
}

// Avoid templates so we do need to include the functions to .cpp files we have
__global__ void copyKernelFloat(const float *a, float *b, size_t N)
{
  const int i = blockDim.x * blockIdx.x + threadIdx.x;
  if (i < N)
  {
    b[i] = a[i];
  }
}

__global__ void copyKernelDouble(const double *a, double *b, size_t N)
{
  const int i = blockDim.x * blockIdx.x + threadIdx.x;
  if (i < N)
  {
    b[i] = a[i];
  }
}

void maxPotentialBlockSize(int* minGridSize, int* blockSize) {
  cudaOccupancyMaxPotentialBlockSize(minGridSize, blockSize, copyKernelFloat, 0, 0);
}

void copyKernelLauncher(const float *a, float *b, size_t N, int gridSize, int blockSize)
{
  copyKernelFloat<<<gridSize, blockSize, 0>>>(a, b, N);
}

#endif