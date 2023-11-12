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

void tarch::accelerator::Device::configure(const std::set<int>& devicesToUse) {
  (void)devicesToUse;
  // TODO: Iterate over devicesToUse and check offloading capability for each device.
  bool canOffload = false;
#pragma omp target map(tofrom : canOffload)
  {
    if (!omp_is_initial_device()) {
      canOffload = true;
    }
  }

  if (not canOffload) {
    logWarning("configure()", "Peano was built with OpenMP GPU offloading support, but the hardware configuration cannot offload to the GPU.");
  }
}

void tarch::accelerator::Device::shutdown() {}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

#endif
