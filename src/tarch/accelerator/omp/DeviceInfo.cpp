#include "DeviceInfo.h"

#if defined(GPUOffloadingOMP)
#include <iostream>
#include <omp.h>
#include <cassert>

#include "../DeviceInfo.h"
#include "Device.h"

namespace tarch::accelerator::info::omp
{
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    unsigned int                          propertyCount   = tarch::accelerator::info::properties.size();
    unsigned int                          numberOfDevices = omp_get_num_devices();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(numberOfDevices, "X"));
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto        propertyMatrix    = queryDeviceInformation();
    int         numberOfDevices   = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::string ompBackendWarning = "OpenMP accelerator back-end has no means to query the accelerators or properties\n"
                                    "For best results enable the CUDA or Sycl Backends/API.\n";
    std::string deviceInfoStr     = numberOfDevices + " offloading capable GPUs present.";
    // This table will be empty
    //std::string deviceInfoStr     = tarch::accelerator::info::parseMatrix(propertyMatrix, 0);
    std::cout << ompBackendWarning << deviceInfoStr << std::endl;
  }

  std::vector<bool> testKernelLaunch() {
    int numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::vector<bool> testResults(numberOfDevices, false);

    for (int i = 0; i< numberOfDevices; i++)
    {
      bool canOffload = false;
      #pragma omp target device(i) map(tofrom : canOffload)
      {
        if (!omp_is_initial_device()){
          canOffload = true;
        }
      }
      testResults[i] = canOffload;
    }

    return testResults;
  }

  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks() {
    int  numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});

    constexpr uint64_t repeats = 20;

    for (int deviceId = 0; deviceId < numberOfDevices; deviceId++)
    {
      tarch::accelerator::info::BenchmarkResultInNanoSeconds& benchmarkResult = benchmarkResults[deviceId];
      benchmarkResult.repeats = repeats;

      // No way to now the amount of free memory without a look up... lets do something not too big
      // size_t free = 1e9;
      // size_t alloc_size = ((free / 2) / (1024*sizeof(float))) * (1024*sizeof(float));
      size_t alloc_size = 1e9;
      size_t N = alloc_size / sizeof(float);
      benchmarkResult.copiedDataSize = alloc_size;
      benchmarkResult.allocatedDataSize = alloc_size * 2;
      for (uint i = 0; i < repeats + 1; i++)
      {
        float* a = new float[N];
        float* b = new float[N];
        std::fill(a, a + N, 0.0f);
        std::fill(b, b + N, 0.0f);
        assert(alloc_size == N * sizeof(float));

        double start = 0.0; 
        double end = 0.0;
        double diff = 0.0;

        start = omp_get_wtime(); 
        void* a_dev = omp_target_alloc(N * sizeof(float), deviceId);
        void* b_dev = omp_target_alloc(N * sizeof(float), deviceId);
        end = omp_get_wtime();
        diff = end - start;
        if (i > 0) { benchmarkResult.GPUMalloc += static_cast<uint64_t>(diff * 1e9); }
        omp_target_free(a_dev, deviceId);
        omp_target_free(b_dev, deviceId);

        start = omp_get_wtime(); 
        #pragma omp target enter data map(to: a[0:N], b[0:N]) device(deviceId)
        {}
        end = omp_get_wtime();
        diff = end - start;
        if (i > 0) { benchmarkResult.CPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9); }

        start = omp_get_wtime(); 
        #pragma omp target teams distribute parallel for simd device(deviceId) schedule(static, 1)
        for (size_t j = 0; j < N; j++)
        {
          b[j] = a[j];
        }
        end = omp_get_wtime();
        diff = end - start;
        if (i > 0) { benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(diff * 1e9); }


        start = omp_get_wtime(); 
        #pragma omp target update from(a[0:N], b[0:N]) device(deviceId)
        {}
        end = omp_get_wtime();
        diff = end - start;
        if (i > 0) { benchmarkResult.GPUtoCPUCopy += static_cast<uint64_t>(diff * 1e9); }

        #pragma omp target exit data map(release: a[0:N], b[0:N]) device(deviceId)
        {}
        
        delete[] a;
        delete[] b;
      }
    }

    return benchmarkResults;
  }
} // namespace tarch::accelerator::info::omp

#endif