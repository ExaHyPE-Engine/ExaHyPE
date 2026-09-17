// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/instrumentation/Info.h"

#include <sycl/sycl.hpp>

namespace {
  std::map<int, sycl::queue> syclQueues;
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
    tarch::terminateWithError("No accelerator devices found. Please check your driver installation.");
  }

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  for (auto& queue : syclQueues) {
    queue.second.wait();
  }
  syclQueues.clear();
}

bool tarch::accelerator::Device::isInitialised() const { return not syclQueues.empty(); }

int tarch::accelerator::Device::getNumberOfDevices() const { return static_cast<int>(syclQueues.size()); }

void* tarch::accelerator::getSYCLQueue(int number) {
  assertion(syclQueues.count(number) == 1, number);
  return &syclQueues[number];
}

#endif // defined(PEANO_WITH_GPU_BACKEND_SYCL)
