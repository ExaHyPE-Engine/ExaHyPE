#include "DeviceInfo.h"

#if defined(GPUOffloadingSYCL)

#include <iostream>
#include <sstream>

#include "../DeviceInfoUtility.h"
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
  std::vector<std::vector<std::string>> queryDeviceInformation()
  {
    constexpr unsigned int        kb      = 1024;
    constexpr unsigned int        mb      = kb * kb;
    std::vector<::sycl::device> devices = ::sycl::device::get_devices(::sycl::info::device_type::gpu);

    unsigned int                          propertyCount =  tarch::accelerator::info::utility::properties.size();
    std::vector<std::vector<std::string>> propertyMatrix(propertyCount, std::vector<std::string>(devices.size(), "X"));

    unsigned int devId = 0;
    for (const auto& dev : devices)
    {
      auto work_item_sizes = dev.get_info<::sycl::info::device::max_work_item_sizes<3>>();
      auto subgroup_sizes  = dev.get_info<::sycl::info::device::sub_group_sizes>();

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

      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Device Name")][devId] = dev.get_info<
        ::sycl::info::device::name>();
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Global Memory")][devId]
        = std::to_string(dev.get_info<::sycl::info::device::global_mem_size>() / mb) + " mb";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Shared Memory")][devId]
        = std::to_string(dev.get_info<::sycl::info::device::local_mem_size>() / kb) + " kb";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Clock Frequency")][devId]
        = std::to_string(dev.get_info<::sycl::info::device::max_clock_frequency>()) + " mHz";
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Compute Units (Streaming Multiprocessors)"
      )][devId]
        = std::to_string(dev.get_info<::sycl::info::device::max_compute_units>());
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Subgroup Size (Warp Size)")][devId] = subgroup;
      propertyMatrix[tarch::accelerator::info::utility::getPropertyOffset("Work Item Sizes (Block Size)")][devId]
        = work_item;

      devId += 1;
    }
    return propertyMatrix;
  }

  void printDeviceInformation()
  {
    auto        propertyMatrix  = queryDeviceInformation();
    int         numberOfDevices = ::sycl::device::get_devices(::sycl::info::device_type::gpu).size();
    std::string deviceInfoStr   = tarch::accelerator::info::utility::parseDeviceInformation(
      propertyMatrix, numberOfDevices
    );
    std::cout << deviceInfoStr << std::endl;
  }
} // namespace tarch::accelerator::info::sycl

#endif