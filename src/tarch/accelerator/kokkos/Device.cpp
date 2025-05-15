// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(GPUOffloadingKokkos)

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/multicore/Lock.h"

#include <Kokkos_Core.hpp>

namespace {
  int numberOfDevices = 0;
} // namespace

bool tarch::accelerator::Device::init() {
  Kokkos::initialize();
  numberOfDevices = Kokkos::num_devices();
  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
  Kokkos::finalize();
}

bool tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

#endif // defined(GPUOffloadingKokkos)
