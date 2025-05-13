// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(GPUOffloadingCPP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/multicore/Lock.h"

bool tarch::accelerator::Device::init() {
  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
}

bool tarch::accelerator::Device::isInitialised() const { return true; }

int tarch::accelerator::Device::getNumberOfDevices() const {
  return 1;
}

#endif // defined(GPUOffloadingCPP)
