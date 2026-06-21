// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/accelerator.h"

#include <map>
#include <sycl/sycl.hpp>

namespace {
  std::map<int, sycl::queue> syclQueues;

  void listSYCLQueues() {
    auto platforms = sycl::platform::get_platforms();
    std::cout << "SYCL system overview:" << std::endl;
    int deviceIndex = 0;
    for (const auto& platform : platforms) {
      std::string platformName = platform.get_info<sycl::info::platform::name>();
      for (const auto& device : platform.get_devices()) {
        std::string deviceName = device.get_info<sycl::info::device::name>();
        std::cout
          << "  Device #" << deviceIndex << " | Platform: " << platformName << " | Device: " << deviceName << std::endl;
        ++deviceIndex;
      }
    }
  }
} // namespace

bool tarch::accelerator::Device::init() {
  auto         platforms           = sycl::platform::get_platforms();
  unsigned int physicalDeviceIndex = 0;
  unsigned int logicalDeviceIndex  = 0;

  syclQueues.clear();

  for (const auto& platform : platforms) {
    std::string platformName = platform.get_info<sycl::info::platform::name>();
    for (const auto& device : platform.get_devices()) {
      std::string deviceName = device.get_info<sycl::info::device::name>();

      // Skip devices that are not GPUs.
      if (device.get_info<sycl::info::device::device_type>() != sycl::info::device_type::gpu) {
        logInfo(
          "Skipping physical device #" + std::to_string(physicalDeviceIndex) + " (" + platformName + " / " + deviceName
          + ") because it is not an accelerator"
        );
        ++physicalDeviceIndex;
        continue;
      }

      logInfo(
        "Initialising physical device #" + std::to_string(physicalDeviceIndex) + " (" + platformName + " / "
        + deviceName + ") as logical device #" + std::to_string(logicalDeviceIndex)
      );
      syclQueues.insert({logicalDeviceIndex, sycl::queue(device)});
      ++logicalDeviceIndex;
      ++physicalDeviceIndex;
    }
  }

  if (syclQueues.empty()) {
    throw std::runtime_error("No accelerator devices found. Please check your driver installation.");
  }

  // listSYCLQueues();

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  for (auto& queue : syclQueues) {
    queue.second.wait();
  }
  MemoryAllocationCache::getInstance().clear();
  syclQueues.clear();
}

bool tarch::accelerator::Device::isInitialised() const { return not syclQueues.empty(); }

int tarch::accelerator::Device::getNumberOfDevices() const { return syclQueues.size(); }

void* tarch::accelerator::getSYCLQueue(int number) {
  assertion(syclQueues.count(number) == 1, number);
  return &syclQueues[number];
}

#endif // defined(PEANO_WITH_GPU_BACKEND_SYCL)
