// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(GPUOffloadingOMP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/multicore/Lock.h"

#include <omp.h>

namespace {
  int numberOfDevices = 0;
} // namespace

tarch::accelerator::Device::Device() {
  numberOfDevices = omp_get_num_devices();
}

void tarch::accelerator::Device::configure(const std::set<int>&) {}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
}

bool tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

#endif // defined(GPUOffloadingOMP)
