#include "tarch/accelerator/Device.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(GPUOffloadingCUDA)
#include <cuda_runtime.h>

namespace {
  int numberOfDevices;
}

tarch::accelerator::Device::Device() {
  cudaGetDeviceCount(&numberOfDevices);
}


tarch::accelerator::Device::~Device() {
}


tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}


void tarch::accelerator::Device::configure( const std::set<int>& devicesToUse ) {
}


void tarch::accelerator::Device::shutdown() {
}


bool tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices>0;
}


int tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

#endif
