#include "tarch/accelerator/Device.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(GPUOffloadingOMP)

#include <omp.h>

bool checkOMPOffloading() {
  bool canOffload = false;
  #pragma omp target map(tofrom : canOffload)
  {
    if (!omp_is_initial_device()) {
      canOffload = true;
    }
  }
  return canOffload;
}

namespace {
  int numberOfDevices;
}

tarch::accelerator::Device::Device() {
  numberOfDevices = omp_get_num_devices();
}


tarch::accelerator::Device::~Device() {
}


tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}


void tarch::accelerator::Device::configure() {
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
