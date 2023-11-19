#include "DeviceInfo.h"

#if defined(GPUOffloadingSYCL)

#include <iostream>
#include <sstream>

#include "../DeviceInfo.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")


namespace tarch::accelerator::sycl {
  void queryDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
    constexpr unsigned int      kb      = 1024;
    constexpr unsigned int      mb      = kb * kb;
    std::vector<::sycl::device> devices = ::sycl::device::get_devices(::sycl::info::device_type::gpu);

    unsigned int                          propertyCount = tarch::accelerator::properties.size();
    std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(devices.size(), "X"));
    propertyMatrix = std::move(_propertyMatrix);

    unsigned int devId = 0;
    for (const auto& dev : devices) {
      auto work_item_sizes = dev.get_info<::sycl::info::device::max_work_item_sizes<3>>();
      auto subgroup_sizes  = dev.get_info<::sycl::info::device::sub_group_sizes>();

      std::stringstream ss;
      if (subgroup_sizes.size() > 1) {
        ss << "<";
        ss << subgroup_sizes.size();
        for (unsigned int i = 0; i < subgroup_sizes.size() - 1; i++) {
          ss << subgroup_sizes[i] << ", ";
        }
        ss << subgroup_sizes[subgroup_sizes.size() - 1] << ">";
      } else if (subgroup_sizes.size() == 1) {
        ss << subgroup_sizes[0] << "";
      }
      std::string subgroup = ss.str();
      ss.str("");

      if (dev.get_info<::sycl::info::device::max_work_item_dimensions>() > 1) {
        ss << "<";
        for (unsigned int i = dev.get_info<::sycl::info::device::max_work_item_dimensions>() - 1; i > 0; i--) {
          ss << work_item_sizes[i] << ", ";
        }
        ss << work_item_sizes[0] << ">";
      } else if (dev.get_info<::sycl::info::device::max_work_item_dimensions>() == 1) {
        ss << work_item_sizes[0] << "";
      }
      std::string work_item = ss.str();
      ss.str("");

      propertyMatrix[tarch::accelerator::getPropertyOffset("Device Name")][devId]     = dev.get_info<::sycl::info::device::name>();
      propertyMatrix[tarch::accelerator::getPropertyOffset("Global Memory")][devId]   = std::to_string(dev.get_info<::sycl::info::device::global_mem_size>() / mb) + " mb";
      propertyMatrix[tarch::accelerator::getPropertyOffset("Clock Frequency")][devId] = std::to_string(dev.get_info<::sycl::info::device::max_clock_frequency>()) + " mHz";
      propertyMatrix[tarch::accelerator::getPropertyOffset("Compute Units")][devId]   = std::to_string(dev.get_info<::sycl::info::device::max_compute_units>());

      devId += 1;
    }
  }

  void printDeviceInformation(std::vector<std::vector<std::string>>& propertyMatrix) {
    int         numberOfDevices = ::sycl::device::get_devices(::sycl::info::device_type::gpu).size();
    std::string deviceInfoStr   = "";
    if (numberOfDevices == 0) {
      deviceInfoStr = "No offloading capable GPUs were found";
      logInfo("printDeviceInformation()", "" << deviceInfoStr);
    } else {
      deviceInfoStr = tarch::accelerator::parseMatrix(propertyMatrix, numberOfDevices);
      logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
    }
  }

  std::vector<bool> testKernelLaunch() {
    logInfo("runBenchmarks()", "Running benchmarks with::sycl backend");

    std::vector<::sycl::device> devices = ::sycl::device::get_devices(::sycl::info::device_type::gpu);
    std::vector<bool>           offloadResults(devices.size(), false);
    bool                        a = false;

    for (unsigned int i = 0; i < devices.size(); i++) {
      ::sycl::device& device = devices[i];
      ::sycl::queue   queue(device);
      std::cout << device.get_info<::sycl::info::device::name>() << std::endl;
      {
        ::sycl::buffer<bool, 1> a_sycl(&a, ::sycl::range<1>(1));

        queue.submit([&](::sycl::handler& cgh) {
          ::sycl::stream   out(1024, 256, cgh);
          ::sycl::accessor a_acc = a_sycl.get_access<::sycl::access::mode::read_write>(cgh);
          // same as::sycl::accessor a_acc{a_sycl, cgh,::sycl::read_write};

          cgh.parallel_for(1, [=](::sycl::id<1> idx) { a_acc[0] = true; });
        });

        queue.wait();
      }

      offloadResults[i] = a;
      a                 = false;
    }

    return offloadResults;
  }

  std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> runBenchmarks() {
    int numberOfDevices = ::sycl::device::get_devices(::sycl::info::device_type::gpu).size();

    constexpr uint64_t repeats = tarch::accelerator::repeats;
    constexpr size_t   N       = tarch::accelerator::N;

    std::vector<float>          a(N, 0.125f);
    std::vector<float>          b(N, 42.0f);
    std::vector<::sycl::device> devices = ::sycl::device::get_devices(::sycl::info::device_type::gpu);
    if (numberOfDevices != devices.size()) {
      logWarning(
        "DeviceInfo()",
        "The number of devices returned by Tarch is not the same as the number of SYCL GPU targets. Tarch: " << numberOfDevices << ", selector device_type::gpu: " << devices.size()
      );
    }

    ::sycl::property_list propList{::sycl::property::queue::enable_profiling()};
    // Enabling Profiling is not slower than using C++ chrono according to my tests

    std::vector<tarch::accelerator::BenchmarkResultInNanoSeconds> benchmarkResults(numberOfDevices, {0});
    for (int deviceId = 0; deviceId < numberOfDevices; deviceId++) {
      tarch::accelerator::BenchmarkResultInNanoSeconds& benchmarkResult = benchmarkResults[deviceId];
      benchmarkResult.repeats                                           = repeats;
      benchmarkResult.allocatedDataSize                                 = 2 * N * sizeof(float);
      ::sycl::device dev                                                = devices[deviceId];
      ::sycl::queue  queue                                              = ::sycl::queue(dev, propList);

      // Create buffers for vectors a and b
      ::sycl::buffer<float, 1> d_a(a.data(), N);
      ::sycl::buffer<float, 1> d_b(b.data(), N);

      try {
        for (uint i = 0; i < repeats + 1; i++) {
          //  Create an event to measure the execution time of the second kernel
          ::sycl::event kernelEvent;

          // Submit a command group for the second kernel to access vectors aCopy and bCopy
          kernelEvent = queue.submit([&](::sycl::handler& cgh) {
            auto ka = d_a.template get_access<::sycl::access::mode::read>(cgh);
            auto kb = d_b.template get_access<::sycl::access::mode::write>(cgh);

            cgh.parallel_for<class copy_kernel>(::sycl::range<1>{N}, [=](::sycl::id<1> idx) { kb[idx] = ka[idx]; });
          });

          // Wait for the second kernel to complete
          kernelEvent.wait();
          // Get the execution time
          auto kernelStartTime     = kernelEvent.get_profiling_info<::sycl::info::event_profiling::command_start>();
          auto kernelEndTime       = kernelEvent.get_profiling_info<::sycl::info::event_profiling::command_end>();
          auto kernelExecutionTime = (kernelEndTime - kernelStartTime); // Convert nanoseconds to seconds

          if (i > 0) {
            // std::cout << kernelExecutionTime << std::endl;
            benchmarkResult.GPUtoGPUCopy += static_cast<uint64_t>(kernelExecutionTime);
          }

          queue.wait();
        }
      } catch (const ::sycl::exception& e) {
        std::cerr
          << "DeviceInfo()"
          << "Error in Vector copy benchmark" << e.what() << std::endl;
      }
    }


    return benchmarkResults;
  }
} // namespace tarch::accelerator::sycl

#endif
