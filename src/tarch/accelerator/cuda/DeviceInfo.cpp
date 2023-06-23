#include "DeviceInfo.h"

#if defined(_NVHPC_CUDA) or defined(GPUOffloadingCUDA)
#include <cuda_runtime.h>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "../DeviceInfo.h"
#include "tarch/Assertions.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(GPUOffloadingCUDA)
std::vector<bool> testKernelLauncher();
void copyKernelLauncher(const float *a, float *b, size_t N);

#define CHECK_CUDA_ERROR()           \
  {                                  \
    check_error(__FILE__, __LINE__); \
  }
void check_error(const char *file, int line)
{
  cudaError_t code = cudaGetLastError();
  if (code != cudaSuccess)
  {
    throw std::runtime_error("CUDA Benchmarks failed with a CUDA error and could not be completed");
  }
}
#endif

namespace tarch::accelerator::cuda
{
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix)
  {
    std::stringstream ss;
    int numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    unsigned int propertyCount = tarch::accelerator::properties.size();
    std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
    propertyMatrix = std::move(_propertyMatrix);

    int defaultDevice;
    cudaGetDevice(&defaultDevice);
    for (unsigned int i = 0; i < numberOfDevices; i++)
    {
      cudaSetDevice(i);
      cudaDeviceProp props;
      cudaGetDeviceProperties(&props, i);
      size_t free, total;
      cudaMemGetInfo(&free, &total);
      constexpr unsigned int kb = 1024;
      constexpr unsigned int mb = kb * kb;
      propertyMatrix[tarch::accelerator::getPropertyOffset("Device Name")][i] = props.name;
      propertyMatrix[tarch::accelerator::getPropertyOffset("Global Memory")][i] = std::to_string(props.totalGlobalMem / mb) + " mb";
      propertyMatrix[tarch::accelerator::getPropertyOffset("Free Memory")][i] = std::to_string(free / mb) + " mb";
      propertyMatrix[tarch::accelerator::getPropertyOffset("Clock Frequency")][i] = std::to_string(props.clockRate / 1000) + " mHz";
      propertyMatrix[tarch::accelerator::getPropertyOffset("Compute Units")][i] = std::to_string(props.multiProcessorCount);
    }
    cudaSetDevice(defaultDevice);
  }

  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix)
  {
    int numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    if (numberOfDevices == 0)
    {
      std::string deviceInfoStr = "No offloading capable GPUs were found";
      logInfo("printDeviceInformation()", "" << deviceInfoStr);
    }
    else
    {
      std::string deviceInfoStr = tarch::accelerator::parseMatrix(
          propertyMatrix, numberOfDevices);
      logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
    }
  }

  std::vector<bool> testKernelLaunch()
  {
#if defined(GPUOffloadingCUDA)
    return testKernelLauncher();
#else
    int numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    return std::vector<bool>(numberOfDevices, false);
#endif
  }

#if defined(GPUOffloadingCUDA)
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks()
  {
    int numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);
    std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});

    constexpr uint64_t repeats = 10;

    for (int deviceId = 0; deviceId < numberOfDevices; deviceId++)
    {
      tarch::accelerator::BenchmarkResultInNanoSeconds &benchmarkResult = benchmarkResults[deviceId];
      benchmarkResult.repeats = repeats;
      for (uint i = 0; i < repeats + 1; i++)
      {
        // Copy Benchmark -> use ~80% of Memory's Available Memory for total array size
        // size_t free;
        // size_t total;
        // cudaMemGetInfo(&free, &total);
        // 100% of free memory will result with an out of memory problem
        // free = free * 0.8;
        // / sizeof(float) then * sizeof(float) for it to be a float array
        // Also a multiply of 1024 because that will be the thread per block count - to avoid using if in kernels
        // size_t alloc_size = ((free / 2) / (1024*sizeof(float))) * (1024*sizeof(float));
        size_t alloc_size = static_cast<size_t>(1e9);
        size_t N = alloc_size / sizeof(float);
        benchmarkResult.readWriteSize = alloc_size * 2;
        benchmarkResult.allocatedDataSize = alloc_size * 2;

        // One can reuse them
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        float executionTime;

        // Malloc memory in CPU
        float *a = new float[N];
        float *b = new float[N];
        std::fill(a, a + N, 0.0f);
        std::fill(b, b + N, 0.0f);
        assert(alloc_size == N * sizeof(float));
        CHECK_CUDA_ERROR();

        // Malloc memory in GPU
        cudaEventRecord(start, 0);
        float *a_dev;
        float *b_dev;
        cudaMalloc((void **)&a_dev, alloc_size);
        cudaMalloc((void **)&b_dev, alloc_size);
        CHECK_CUDA_ERROR();
        cudaEventRecord(stop, 0);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&executionTime, start, stop);
        if (i > 0)
        {
          benchmarkResult.GPUMalloc += static_cast<uint64_t>(executionTime * 1e6);
        }
        CHECK_CUDA_ERROR();

        // Copy from CPU to GPU
        cudaEventRecord(start, 0);
        cudaMemcpy(a_dev, a, alloc_size, cudaMemcpyHostToDevice);
        cudaMemcpy(b_dev, b, alloc_size, cudaMemcpyHostToDevice);
        cudaEventRecord(stop, 0);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&executionTime, start, stop);
        if (i > 0)
        {
          benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(executionTime * 1e6);
        }
        CHECK_CUDA_ERROR();

        // Copy Kernel inside GPU
        cudaEventRecord(start, 0);
        copyKernelLauncher(a_dev, b_dev, N);
        cudaDeviceSynchronize();
        cudaEventRecord(stop, 0);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&executionTime, start, stop);
        if (i > 0)
        {
          benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(executionTime * 1e6);
        }
        CHECK_CUDA_ERROR();

        // Copy from GPU to CPU
        cudaEventRecord(start, 0);
        cudaMemcpy(a, a_dev, alloc_size, cudaMemcpyDeviceToHost);
        cudaMemcpy(b, b_dev, alloc_size, cudaMemcpyDeviceToHost);
        cudaEventRecord(stop, 0);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&executionTime, start, stop);
        if (i > 0)
        {
          benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(executionTime * 1e6);
        }
        CHECK_CUDA_ERROR();

        cudaFree(a_dev);
        cudaFree(b_dev);
        CHECK_CUDA_ERROR();
        delete[] a;
        delete[] b;
      }
    }

    return benchmarkResults;
  }
#endif
} // namespace tarch::accelerator::cuda

#endif