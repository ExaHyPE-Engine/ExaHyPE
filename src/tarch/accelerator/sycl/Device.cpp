// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(GPUOffloadingSYCL)

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Lock.h"

#include <map>

#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")

namespace {
  std::map<int, sycl::queue> syclQueues;

  void listSYCLQueues() {
    auto platforms = sycl::platform::get_platforms();
    std::cout << "SYCL system overview:" << std::endl;
    int deviceIndex = 0;
    for (const auto &platform : platforms) {
      std::string platformName = platform.get_info<sycl::info::platform::name>();
      for (const auto &device : platform.get_devices()) {
        std::string deviceName = device.get_info<sycl::info::device::name>();
        std::cout << "  Device #" << deviceIndex
                  << " | Platform: " << platformName
                  << " | Device: " << deviceName << std::endl;
        ++deviceIndex;
      }
    }
  }
} // namespace

tarch::accelerator::Device::Device() {}

void tarch::accelerator::Device::configure(const std::set<int>& devicesToUse) {
  auto platforms                    = sycl::platform::get_platforms();
  unsigned int physicalDeviceIndex  = 0;
  unsigned int logicalDeviceIndex   = 0;

  syclQueues.clear();

  for (const auto& platform : platforms) {
    std::string platformName = platform.get_info<sycl::info::platform::name>();
    for (const auto &device : platform.get_devices()) {
      std::string deviceName = device.get_info<sycl::info::device::name>();

       // Skip devices that are not GPUs.
       if (device.get_info<sycl::info::device::device_type>() != sycl::info::device_type::gpu) {
        logInfo("configure()",
                "Skipping physical device #" + std::to_string(physicalDeviceIndex) +
                " (" + platformName + " / " + deviceName +
                ") because it is not an accelerator");
        ++physicalDeviceIndex;
        continue;
      }

      if (devicesToUse.empty() || devicesToUse.count(physicalDeviceIndex) > 0) {
        logInfo("configure()",
                "Initialising physical device #" + std::to_string(physicalDeviceIndex) +
                " (" + platformName + " / " + deviceName +
                ") as logical device #" + std::to_string(logicalDeviceIndex));
        syclQueues.insert({logicalDeviceIndex, sycl::queue(device)});
        ++logicalDeviceIndex;
      } else {
        logInfo("configure()",
                "Skipping physical device #" + std::to_string(physicalDeviceIndex) +
                " (masked out)");
      }
      ++physicalDeviceIndex;
    }
  }

  if (syclQueues.empty()) {
    throw std::runtime_error("No accelerator devices found. Please check your driver installation.");
  }

  // listSYCLQueues();
}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
  syclQueues.clear();
}

bool tarch::accelerator::Device::isInitialised() const {
  return not syclQueues.empty();
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return syclQueues.size();
}

void* tarch::accelerator::getSYCLQueue(int number) {
  assertion1(syclQueues.count(number) == 1, number);
  return &syclQueues[number];
}

#endif // defined(GPUOffloadingSYCL)
