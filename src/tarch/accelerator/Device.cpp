// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#include <omp.h>

tarch::accelerator::Device&
tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

#if not defined(PEANO_WITH_GPU_BACKEND)

bool
tarch::accelerator::Device::init() {
  return isInitialised();
}

void
tarch::accelerator::Device::shutdown() {}

bool
tarch::accelerator::Device::isInitialised() const {
  return true;
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return 0;
}

/**
 * Returns the threads of the host that run a dispatch at once.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  return std::max(1, omp_get_max_threads());
}

/**
 * Returns std::nullopt. A build without a GPU backend sends no device buffers.
 */
std::optional<bool>
tarch::accelerator::Device::isMPIGPUAware() const {
  return std::nullopt;
}

/**
 * Returns an empty string. A build without a GPU backend runs on the host only.
 */
std::string
tarch::accelerator::Device::getIdOfCurrentDevice() const {
  return {};
}

#endif // PEANO_WITH_GPU_BACKEND
