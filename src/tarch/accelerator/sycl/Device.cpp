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

bool
tarch::accelerator::Device::init() {
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
    tarch::terminateWithError("The SYCL runtime needs an accelerator device with a working driver.");
  }

  return isInitialised();
}

void
tarch::accelerator::Device::shutdown() {
  for (auto& queue : syclQueues) {
    queue.second.wait();
  }
  tarch::accelerator::releaseReductionScratch();
  syclQueues.clear();
}

bool
tarch::accelerator::Device::isInitialised() const {
  return not syclQueues.empty();
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return static_cast<int>(syclQueues.size());
}

/**
 * Returns the work items the current device runs at once: Its compute units times the largest
 * work group one compute unit holds.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  const sycl::device device = syclQueues[tarch::accelerator::getCurrentDevice()].get_device();
  const auto         units  = static_cast<int>(device.get_info<sycl::info::device::max_compute_units>());
  const auto         items  = static_cast<int>(device.get_info<sycl::info::device::max_work_group_size>());
  return std::max(1, units * items);
}

/**
 * Returns std::nullopt. A SYCL build tells no device vendor that an MPI query names.
 */
std::optional<bool>
tarch::accelerator::Device::isMPIGPUAware() const {
  return std::nullopt;
}

/**
 * Returns an empty string. SYCL has no physical address of a device.
 */
std::string
tarch::accelerator::Device::getIdOfCurrentDevice() const {
  return {};
}

/**
 * Returns the queue of a logical device number. The logical number counts the GPUs only and
 * differs from the physical device number.
 */
void*
tarch::accelerator::getSYCLQueue(int number) {
  assertion(syclQueues.count(number) == 1, number);
  return &syclQueues[number];
}

#endif // defined(PEANO_WITH_GPU_BACKEND_SYCL)
