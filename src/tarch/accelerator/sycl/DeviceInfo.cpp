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

namespace tarch::accelerator::info::sycl
{
  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix)
  {
    constexpr unsigned int kb = 1024;
    constexpr unsigned int mb = kb * kb;
    std::vector<::sycl::device> devices = ::sycl::device::get_devices();

    unsigned int propertyCount = tarch::accelerator::info::properties.size();
    std::vector<std::vector<std::string>> _propertyMatrix(propertyCount, std::vector<std::string>(devices.size(), "X"));
    propertyMatrix = std::move(_propertyMatrix);

    unsigned int devId = 0;
    for (const auto &dev : devices)
    {
      auto work_item_sizes = dev.get_info<::sycl::info::device::max_work_item_sizes<3>>();
      auto subgroup_sizes = dev.get_info<::sycl::info::device::sub_group_sizes>();

      std::stringstream ss;
      if (subgroup_sizes.size() > 1)
      {
        ss << "<";
        ss << subgroup_sizes.size();
        for (unsigned int i = 0; i < subgroup_sizes.size() - 1; i++)
        {
          ss << subgroup_sizes[i] << ", ";
        }
        ss << subgroup_sizes[subgroup_sizes.size() - 1] << ">";
      }
      else if (subgroup_sizes.size() == 1)
      {
        ss << subgroup_sizes[0] << "";
      }
      std::string subgroup = ss.str();
      ss.str("");

      if (dev.get_info<::sycl::info::device::max_work_item_dimensions>() > 1)
      {
        ss << "<";
        for (unsigned int i = dev.get_info<::sycl::info::device::max_work_item_dimensions>() - 1; i > 0; i--)
        {
          ss << work_item_sizes[i] << ", ";
        }
        ss << work_item_sizes[0] << ">";
      }
      else if (dev.get_info<::sycl::info::device::max_work_item_dimensions>() == 1)
      {
        ss << work_item_sizes[0] << "";
      }
      std::string work_item = ss.str();
      ss.str("");

      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Device Name")][devId] = dev.get_info<
          ::sycl::info::device::name>();
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Global Memory")][devId] = std::to_string(dev.get_info<::sycl::info::device::global_mem_size>() / mb) + " mb";
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Clock Frequency")][devId] = std::to_string(dev.get_info<::sycl::info::device::max_clock_frequency>()) + " mHz";
      propertyMatrix[tarch::accelerator::info::getPropertyOffset("Compute Units")][devId] = std::to_string(dev.get_info<::sycl::info::device::max_compute_units>());

      devId += 1;
    }
  }

  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix)
  {
    int numberOfDevices = ::sycl::device::get_devices(::sycl::info::device_type::gpu).size();
    std::string deviceInfoStr = "";
    if (numberOfDevices == 0)
    {
      deviceInfoStr = "No offloading capable GPUs were found";
      logInfo("printDeviceInformation()", "" << deviceInfoStr);
    }
    else
    {
      deviceInfoStr = tarch::accelerator::info::parseMatrix(
          propertyMatrix, numberOfDevices);
      logInfo("printDeviceInformation()", "\n" << deviceInfoStr);
    }
  }

  std::vector<bool> testKernelLaunch()
  {
    std::vector<::sycl::device> devices = ::sycl::device::get_devices(::sycl::info::device_type::gpu);
    std::vector<bool> offloadResults(devices.size(), false);
    bool a = false;

    for (unsigned int i = 0; i < devices.size(); i++)
    {
      ::sycl::device &device = devices[i];
      ::sycl::queue queue(device);
      ::sycl::buffer<bool, 1> a_sycl(&a, ::sycl::range<1>(1));
      std::cout << device.get_info<::sycl::info::device::name>() << std::endl;

      queue.submit([&](::sycl::handler &cgh)
                   {
        ::sycl::stream out(1024, 256, cgh);
        ::sycl::accessor a_acc = a_sycl.get_access<::sycl::access::mode::read_write>(cgh);
        //same as ::sycl::accessor a_acc{a_sycl, cgh, ::sycl::read_write};

        cgh.parallel_for(1, [=] (::sycl::id<1> idx) {
          a_acc[0] = true;
        }); });

      queue.wait();

      offloadResults[i] = a;
      a = false;
    }

    return offloadResults;
  }

  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks()
  {
    return std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds>();
  }
} // namespace tarch::accelerator::info::sycl

#endif