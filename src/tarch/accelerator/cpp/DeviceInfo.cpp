// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceInfo.h"

#if defined(GPUOffloadingCPP)

#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <ranges>

#include "tarch/accelerator/DeviceInfo.h"

void tarch::accelerator::cpp::queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  unsigned int                          propertyCount = tarch::accelerator::Properties.size();
  std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(1, "X"));
  propertyMatrix = std::move(_propertyMatrix);
}


void tarch::accelerator::cpp::printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  std::string deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, 1);
  logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
}


std::vector<bool> tarch::accelerator::cpp::testKernelLaunch() {
#if !defined(_NVHPC_STDPAR_GPU)
  return std::vector<bool>(1, false);
#else
  std::vector<bool> v;
  return v;
#endif
}


std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> tarch::accelerator::cpp::runBenchmarks() {
  constexpr auto                                                NumberOfRepeats = tarch::accelerator::NumberOfRepeats;
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(1, {0});
  tarch::accelerator::BenchmarkResultInNanoSeconds&             benchmarkResult = benchmarkResults[0];
  benchmarkResult.repeats                                                       = NumberOfRepeats;
  constexpr auto AllocSize                                                      = tarch::accelerator::AllocSize;
  constexpr auto N                                                              = tarch::accelerator::N;
  benchmarkResult.readWriteSize                                                 = AllocSize * 2;
  benchmarkResult.allocatedDataSize                                             = AllocSize * 2;

  double* a = new double[N];
  double* b = new double[N];
  std::fill(a, a + N, 0.0);
  std::fill(b, b + N, 0.0);

  auto begin = std::chrono::high_resolution_clock::now();
  std::for_each_n(
    std::execution::par_unseq,
    std::views::iota(0).begin(),
    N, // loop range
    [=](int i) {
      a[i] = 0.125;
      b[i] = 0.250;
    }
  );
  auto end  = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(diff);
  benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(diff);

  for (uint i = 0; i < NumberOfRepeats + 1; i++) {
    begin = std::chrono::high_resolution_clock::now();
    std::for_each_n(std::execution::par_unseq, std::views::iota(0).begin(), N, [=](int i) { b[i] = a[i]; });
    end  = std::chrono::high_resolution_clock::now();
    diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    if (i > 0) {
      benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(diff);
    }
  }

  delete[] a;
  delete[] b;

  return benchmarkResults;
}

#endif
