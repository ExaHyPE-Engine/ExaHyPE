
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

void copyKernelLauncher(const float *a, float *b, size_t N)
{
  static tarch::logging::Log _log("tarch::accelerator::cuda::kernel");

  static int called = 0;
  int blockSize;   // The launch configurator returned block size
  int minGridSize; // The minimum grid size needed to achieve the
                   // maximum occupancy for a full device launch
  int gridSize;    // The actual grid size needed, based on input size

  cudaOccupancyMaxPotentialBlockSize(&minGridSize, &blockSize,
                                     copyKernelFloat, 0, 0);
  // Round up according to array size
  gridSize = (N + blockSize - 1) / blockSize;

  if (called == 0)
  {
    logInfo("copyKernel()", "gridSize: " << gridSize << ", blockSize: " << blockSize);
  }
  copyKernelFloat<<<gridSize, blockSize>>>(a, b, N);
  called++;
}

#endif