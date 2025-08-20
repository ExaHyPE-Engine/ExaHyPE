// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

tarch::logging::Log tarch::accelerator::Device::_log("tarch::accelerator::Device");

tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

#if !defined(WITH_GPU)

bool tarch::accelerator::Device::init() { return isInitialised(); }

void tarch::accelerator::Device::shutdown() {}

bool tarch::accelerator::Device::isInitialised() const { return true; }

int tarch::accelerator::Device::getNumberOfDevices() const { return 0; }

#endif // WITH_GPU
