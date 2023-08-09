#include "DeviceInfo.h"

#if defined(GPUOffloadingOMP)
#include <cassert>
#include <iostream>
#include <omp.h>

#include "Device.h"
#include "../DeviceInfo.h"

namespace tarch::accelerator::omp {
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
    unsigned int                          propertyCount   = tarch::accelerator::properties.size();
    unsigned int                          numberOfDevices = omp_get_num_devices();
    std::vector<std::vector<std::string>> _propertyMatrix(
      propertyCount, std::vector<std::string>(numberOfDevices, "X")
    );
    propertyMatrix = std::move(_propertyMatrix);
  }

  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
    int         numberOfDevices   = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::string ompBackendWarning = "OpenMP accelerator back-end has no means to query the accelerators' properties. "
                                    "For best results enable the CUDA or Sycl Backend/API.";
    std::string deviceInfoStr     = "No offloading capable GPUs were found";
    logInfo("printDeviceInformation()", " " << ompBackendWarning);
    if (numberOfDevices == 0) {
      logInfo("printDeviceInformation()", "" << deviceInfoStr);
    } else {
      deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, numberOfDevices);
      logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
    }
  }

  std::vector<bool> testKernelLaunch() {
    int               numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::vector<bool> testResults(numberOfDevices, false);

    for (int i = 0; i < numberOfDevices; i++) {
      bool canOffload = false;
#pragma omp target                                    device(i) map(tofrom : canOffload)
      {
        if (!omp_is_initial_device()) {
          canOffload = true;
        }
      }
      testResults[i] = canOffload;
    }

    return testResults;
  }

  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks() {
    logInfo("runBenchmarks()", "Running benchmarks with OpenMP backend");

    int numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});

    constexpr uint64_t repeats = tarch::accelerator::repeats;

    for (int deviceId = 0; deviceId < numberOfDevices; deviceId++) {
      tarch::accelerator::BenchmarkResultInNanoSeconds& benchmarkResult = benchmarkResults[deviceId];
      benchmarkResult.repeats = repeats;

      // No way to now the amount of free memory without a look up... lets do something not too big
      // size_t free = 1e9;
      // size_t alloc_size = ((free / 2) / (1024*sizeof(float))) * (1024*sizeof(float));
      size_t alloc_size                 = tarch::accelerator::alloc_size;
      size_t N                          = tarch::accelerator::N;
      benchmarkResult.readWriteSize     = alloc_size * 2;
      benchmarkResult.allocatedDataSize = alloc_size * 2;
      for (uint i = 0; i < repeats + 1; i++) {
        float* a = new float[N];
        float* b = new float[N];
        std::fill(a, a + N, 0.0f);
        std::fill(b, b + N, 0.0f);
        assert(alloc_size == N * sizeof(float));

        double start = 0.0;
        double end   = 0.0;
        double diff  = 0.0;

        /*
        // It doesnt always return NULL when not enough memory
        // For right now just allocate ~1 GB
        start = omp_get_wtime();
        void *a_dev = omp_target_alloc(N * sizeof(float), deviceId);
        void *b_dev = omp_target_alloc(N * sizeof(float), deviceId);
        end = omp_get_wtime();
        diff = end - start;
        if (i > 0)
        {
          benchmarkResult.GPUMalloc += static_cast<uint64_t>(diff * 1e9);
        }
        omp_target_free(a_dev, deviceId);
        omp_target_free(b_dev, deviceId);
        */

        start = omp_get_wtime();
#pragma omp target enter data map(to : a[0 : N], b[0 : N]) device(deviceId)
        {}
        end  = omp_get_wtime();
        diff = end - start;
        if (i > 0) {
          benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9);
        }

        start = omp_get_wtime();
#pragma omp target teams distribute parallel for simd device(deviceId) schedule(static, 1)
        for (size_t j = 0; j < N; j++) {
          b[j] = a[j];
        }
        end  = omp_get_wtime();
        diff = end - start;
        if (i > 0) {
          benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9);
        }

        start = omp_get_wtime();
#pragma omp target update from(a[0 : N], b[0 : N]) device(deviceId)
        {}
        end  = omp_get_wtime();
        diff = end - start;
        if (i > 0) {
          benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(diff * 1e9);
        }

#pragma omp target exit data map(release : a[0 : N], b[0 : N]) device(deviceId)
        {}

#pragma omp target exit data map(delete : a[0 : N], b[0 : N]) device(deviceId)
        {
        }

        delete[] a;
        delete[] b;
      }
    }

    return benchmarkResults;
  }
} // namespace tarch::accelerator::omp

#endif