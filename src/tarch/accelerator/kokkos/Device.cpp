// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(WITH_GPU_KOKKOS)

#include <Kokkos_Core.hpp>

namespace {
  int numberOfDevices = 0;
} // namespace

bool tarch::accelerator::Device::init() {
  Kokkos::initialize();
  numberOfDevices = Kokkos::num_devices();

  logDebug("-----------------------------------------");
  std::stringstream ss;
  Kokkos::print_configuration(ss, true); // 'true' enables verbose details

  std::string line;
  while (std::getline(ss, line)) {
    if (!line.empty()) {
      logDebug(line);
    }
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() { Kokkos::finalize(); }

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

#endif // defined(WITH_GPU_KOKKOS)
