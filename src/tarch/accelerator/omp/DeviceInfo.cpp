// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceInfo.h"

#if defined(GPUOffloadingOMP)

#include <cassert>
#include <iostream>
#include <omp.h>

#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/DeviceInfo.h"

void tarch::accelerator::omp::queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  const unsigned int                          propertyCount   = tarch::accelerator::Properties.size();
  const unsigned int                          numberOfDevices = omp_get_num_devices();
  const std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
  propertyMatrix = std::move(_propertyMatrix);
}


void tarch::accelerator::omp::printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
  const int numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
  if (numberOfDevices == 0) {
    const std::string deviceInfoStr = "No offloading capable GPUs were found";
    logInfo("printDeviceInformation()", "" << deviceInfoStr);
  } else {
    const std::string deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, numberOfDevices);
    logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
  }
}


std::vector<bool> tarch::accelerator::omp::testKernelLaunch() {
  const int         numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
  std::vector<bool> testResults(numberOfDevices, false);

  for (int i = 0; i < numberOfDevices; i++) {
    bool canOffload = false;
#pragma omp target device(i) map(tofrom : canOffload)
    {
      if (!omp_is_initial_device()) {
        canOffload = true;
      }
    }
    testResults[i] = canOffload;
  }

  return testResults;
}


std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> tarch::accelerator::omp::runBenchmarks() {
  const int                                                     numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});

  constexpr auto NumberOfRepeats = tarch::accelerator::NumberOfRepeats;

  for (int deviceId = 0; deviceId < numberOfDevices; deviceId++) {
    tarch::accelerator::BenchmarkResultInNanoSeconds& benchmarkResult = benchmarkResults[deviceId];
    benchmarkResult.repeats                                           = NumberOfRepeats;

    constexpr auto AllocSize          = tarch::accelerator::AllocSize;
    constexpr auto N                  = tarch::accelerator::N;
    benchmarkResult.readWriteSize     = AllocSize * 2;
    benchmarkResult.allocatedDataSize = AllocSize * 2;

    double* a = new double[N];
    double* b = new double[N];
    std::fill(a, a + N, 0.0f);
    std::fill(b, b + N, 0.0f);
    static_assert(AllocSize == N * sizeof(double));

    double start = 0.0;
    double end   = 0.0;
    double diff  = 0.0;

    // clang-format off
    start = omp_get_wtime();
    #pragma omp target enter data map(to : a[0 : N], b[0 : N]) device(deviceId)
    {}
    end  = omp_get_wtime();
    diff = end - start;
    benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9);

    for (uint i = 0; i < NumberOfRepeats + 1; i++) {
      start = omp_get_wtime();
      #pragma omp target teams distribute parallel for simd device(deviceId)
      for (std::size_t j = 0; j < N; j++) {
        b[j] = a[j];
      }
      end  = omp_get_wtime();
      diff = end - start;
      if (i > 0) {
        benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9);
      }
    }

    start = omp_get_wtime();
    #pragma omp target update from(a[0 : N], b[0 : N]) device(deviceId)
    {}
    end  = omp_get_wtime();
    diff = end - start;
    benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(diff * 1e9);

    #pragma omp target exit data map(release : a[0 : N], b[0 : N]) device(deviceId)
    {}

    #pragma omp target exit data map(delete : a[0 : N], b[0 : N]) device(deviceId)
    {}
    // clang-format on

    delete[] a;
    delete[] b;
  }

  return benchmarkResults;
}

#endif
