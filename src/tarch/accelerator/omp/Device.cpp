// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(GPUOffloadingOMP)

#include <omp.h>

namespace {
  int numberOfDevices = 0;
} // namespace

tarch::accelerator::Device::Device() { numberOfDevices = omp_get_num_devices(); }

tarch::accelerator::Device::~Device() {}

tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

void tarch::accelerator::Device::configure(const std::set<int>&) {}

void tarch::accelerator::Device::shutdown() {}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

#endif
