// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include "tarch/instrumentation/Debug.h"

#include <Kokkos_Core.hpp>

namespace {
  int numberOfDevices = 0;
} // namespace

bool
tarch::accelerator::Device::init() {
  Kokkos::initialize();
  numberOfDevices = Kokkos::num_devices();

  logDebug("-----------------------------------------");
  std::stringstream configuration;
  Kokkos::print_configuration(configuration, true);

  std::string line;
  while (std::getline(configuration, line)) {
    if (not line.empty()) {
      logDebug(line);
    }
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

void
tarch::accelerator::Device::shutdown() {
  Kokkos::fence();
  Kokkos::finalize();
  numberOfDevices = 0;
}

bool
tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

/**
 * Returns the threads the default execution space runs at once.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  return std::max(1, static_cast<int>(Kokkos::DefaultExecutionSpace().concurrency()));
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

#endif // defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
