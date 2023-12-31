// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceInfo.h"

#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)

#include <algorithm>
#include <cuda_runtime.h>
#include <iostream>
#include <sstream>

#include "ErrorCheck.h"
#include "tarch/Assertions.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#if defined(GPUOffloadingCUDA)
std::vector<bool> testKernelLauncher();
void              copyKernelLauncher(const double* a, double* b, size_t N, int gridSize, int blockSize);
void              maxPotentialBlockSize(int* minGridSize, int* blockSize);
#endif


void tarch::accelerator::cuda::queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  std::stringstream ss;
  int               numberOfDevices = 0;
  cudaGetDeviceCount(&numberOfDevices);
  unsigned int                          propertyCount = tarch::accelerator::properties.size();
  std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
  propertyMatrix = std::move(_propertyMatrix);

  int defaultDevice = -1;
  cudaGetDevice(&defaultDevice);
  for (unsigned int i = 0; i < numberOfDevices; i++) {
    cudaSetDevice(i);
    cudaDeviceProp props;
    cudaGetDeviceProperties(&props, i);
    std::size_t free, total;
    cudaMemGetInfo(&free, &total);
    constexpr unsigned int kb                                                   = 1024;
    constexpr unsigned int mb                                                   = kb * kb;
    propertyMatrix[tarch::accelerator::getPropertyOffset("Device Name")][i]     = props.name;
    propertyMatrix[tarch::accelerator::getPropertyOffset("Global Memory")][i]   = std::to_string(props.totalGlobalMem / mb) + " mb";
    propertyMatrix[tarch::accelerator::getPropertyOffset("Free Memory")][i]     = std::to_string(free / mb) + " mb";
    propertyMatrix[tarch::accelerator::getPropertyOffset("Clock Frequency")][i] = std::to_string(props.clockRate / 1000) + " mHz";
    propertyMatrix[tarch::accelerator::getPropertyOffset("Compute Units")][i]   = std::to_string(props.multiProcessorCount);
  }
  cudaSetDevice(defaultDevice);
}


void tarch::accelerator::cuda::printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  int numberOfDevices = 0;
  cudaGetDeviceCount(&numberOfDevices);
  if (numberOfDevices == 0) {
    std::string deviceInfoStr = "No offloading capable GPUs were found";
    logInfo("printDeviceInformation()", "" << deviceInfoStr);
  } else {
    std::string deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, numberOfDevices);
    logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
  }
}


std::vector<bool> testKernelLaunch() {
#if defined(GPUOffloadingCUDA)
  return testKernelLauncher();
#else
  int numberOfDevices;
  cudaGetDeviceCount(&numberOfDevices);
  return std::vector<bool>(numberOfDevices, false);
#endif
}


#if defined(GPUOffloadingCUDA)
std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> tarch::accelerator::cuda::runBenchmarks() {
  int numberOfDevices = 0;
  CHECK_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});

  constexpr auto NumberOfRepeats = tarch::accelerator::NumberOfRepeats;
  int            currentDevice   = -1;
  CHECK_CUDA_ERROR(cudaGetDevice(&currentDevice));

  for (int deviceId = 0; deviceId < numberOfDevices; deviceId++) {
    CHECK_CUDA_ERROR(cudaSetDevice(deviceId));
    tarch::accelerator::BenchmarkResultInNanoSeconds& benchmarkResult = benchmarkResults[deviceId];
    benchmarkResult.repeats                                           = NumberOfRepeats;

    int blockSize   = 0;
    int minGridSize = 0; // The minimum grid size needed to achieve the
                         // maximum occupancy for a full device launch.
    int gridSize = 0;    // The actual grid size needed, based on input size.

    // maxPotentialBlockSize(&minGridSize, &blockSize);

    blockSize = 256;

    // Round up according to array size
    gridSize = (N + blockSize - 1) / blockSize;

    cudaEvent_t startCPUGPU, stopCPUGPU;
    cudaEvent_t startGPUCPU, stopGPUCPU;
    cudaEvent_t startGPUMalloc, stopGPUMalloc;
    float       executionTimeCPUGPU;
    float       executionTimeGPUCPU;
    float       executionTimeGPUMalloc;
    cudaEventCreate(&startCPUGPU);
    cudaEventCreate(&stopCPUGPU);
    cudaEventCreate(&startGPUCPU);
    cudaEventCreate(&stopGPUCPU);
    cudaEventCreate(&startGPUMalloc);
    cudaEventCreate(&stopGPUMalloc);
    CHECK_CUDA_LAST_ERROR();

    // Malloc memory in CPU
    double* a = new double[N];
    double* b = new double[N];
    std::fill(a, a + N, 0.0);
    std::fill(b, b + N, 0.0);
    assert(alloc_size == N * sizeof(double));

    // Malloc memory in GPU
    cudaEventRecord(startGPUMalloc, 0);
    double* a_dev;
    double* b_dev;
    CHECK_CUDA_ERROR(cudaMalloc((void**)&a_dev, alloc_size));
    CHECK_CUDA_ERROR(cudaMalloc((void**)&b_dev, alloc_size));
    cudaEventRecord(stopGPUMalloc, 0);
    cudaEventSynchronize(stopGPUMalloc);
    cudaEventElapsedTime(&executionTimeGPUMalloc, startGPUMalloc, stopGPUMalloc);
    benchmarkResult.GPUMalloc += static_cast<uint64_t>(executionTimeGPUMalloc * 1e6);

    // Copy from CPU to GPU
    cudaEventRecord(startCPUGPU, 0);
    cudaMemcpy(a_dev, a, alloc_size, cudaMemcpyHostToDevice);
    cudaMemcpy(b_dev, b, alloc_size, cudaMemcpyHostToDevice);
    cudaEventRecord(stopCPUGPU, 0);
    cudaEventSynchronize(stopCPUGPU);
    cudaEventElapsedTime(&executionTimeCPUGPU, startCPUGPU, stopCPUGPU);
    benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(executionTimeCPUGPU * 1e6);
    CHECK_CUDA_LAST_ERROR();

    for (uint i = 0; i < NumberOfRepeats + 1; i++) {
      std::size_t AllocSize             = tarch::accelerator::AllocSize;
      std::size_t N                     = tarch::accelerator::N;
      benchmarkResult.readWriteSize     = AllocSize * 2;
      benchmarkResult.allocatedDataSize = AllocSize * 2;

      // One can reuse them
      cudaEvent_t startGPUGPU, stopGPUGPU;
      float       executionTimeGPUGPU;
      cudaEventCreate(&startGPUGPU);
      cudaEventCreate(&stopGPUGPU);

      cudaEventRecord(startGPUGPU, 0);
      copyKernelLauncher(a_dev, b_dev, N, gridSize, blockSize);
      cudaEventRecord(stopGPUGPU, 0);
      cudaEventSynchronize(stopGPUGPU);
      cudaEventElapsedTime(&executionTimeGPUGPU, startGPUGPU, stopGPUGPU);
      if (i > 0) {
        benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(executionTimeGPUGPU * 1e6);
      }
      CHECK_CUDA_LAST_ERROR();
    }

    // Copy from GPU to CPU
    cudaEventRecord(startGPUCPU, 0);
    cudaMemcpy(a, a_dev, alloc_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(b, b_dev, alloc_size, cudaMemcpyDeviceToHost);
    cudaEventRecord(stopGPUCPU, 0);
    cudaEventSynchronize(stopGPUCPU);
    cudaEventElapsedTime(&executionTimeGPUCPU, startGPUCPU, stopGPUCPU);
    benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(executionTimeGPUCPU * 1e6);
    CHECK_CUDA_LAST_ERROR();

    cudaFree(a_dev);
    cudaFree(b_dev);
    CHECK_CUDA_LAST_ERROR();
    delete[] a;
    delete[] b;
  }


  cudaSetDevice(currentDevice);
  CHECK_CUDA_LAST_ERROR();

  return benchmarkResults;
}
#endif

#endif
