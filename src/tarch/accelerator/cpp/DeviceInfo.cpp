#include "DeviceInfo.h"

#if defined(GPUOffloadingCPP)
#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <ranges>

#include "../DeviceInfo.h"

namespace tarch::accelerator::cpp {
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
    unsigned int                          propertyCount = tarch::accelerator::properties.size();
    std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(1, "X"));
    propertyMatrix = std::move(_propertyMatrix);
  }

  void printDeviceInformation([[maybe_unused]] std::vector<std::vector<std::string>>& propertyMatrix) {
    logInfo(
      "printDeviceInformation()",
      "Iso cpp accelerator back-end has no means to query the number of accelerators' properties"
    );
    std::string deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, 1);
    logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
  }

  std::vector<bool> testKernelLaunch() {
#if !defined(_NVHPC_STDPAR_GPU)
    return std::vector<bool>(1, false);
#else
    std::vector<bool> v;
    return v;
#endif
  }

  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks() {
    logInfo("runBenchmarks()", "Running benchmarks with ISO C++ backend");
    logInfo("runBenchmarks()", "ISO C++ does not expose devices");

    constexpr uint64_t                                            repeats = tarch::accelerator::repeats;
    std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(1, {0});
    tarch::accelerator::BenchmarkResultInNanoSeconds&             benchmarkResult = benchmarkResults[0];
    benchmarkResult.repeats                                                       = repeats;
    size_t alloc_size                                                             = tarch::accelerator::alloc_size;
    size_t N                                                                      = tarch::accelerator::N;
    benchmarkResult.readWriteSize                                                 = alloc_size * 2;
    benchmarkResult.allocatedDataSize                                             = alloc_size * 2;

    for (uint i = 0; i < repeats + 1; i++) {
      float* a = new float[N];
      float* b = new float[N];
      std::fill(a, a + N, 0.0f);
      std::fill(b, b + N, 0.0f);

      auto begin = std::chrono::high_resolution_clock::now();
      std::for_each_n(
        std::execution::par_unseq,
        std::views::iota(0).begin(),
        N, // loop range
        [=](int i) {
          a[i] = 0.0f;
          b[i] = 0.0f;
        }
      );
      auto end  = std::chrono::high_resolution_clock::now();
      auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
      if (i > 0) {
        benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(diff);
        benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(diff);
      }

      begin = std::chrono::high_resolution_clock::now();
      std::for_each_n(std::execution::par_unseq, std::views::iota(0).begin(), N, [=](int i) { b[i] = a[i]; });
      end  = std::chrono::high_resolution_clock::now();
      diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
      if (i > 0) {
        benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(diff);
      }

      delete[] a;
      delete[] b;
    }

    return benchmarkResults;
  }
} // namespace tarch::accelerator::cpp
#endif