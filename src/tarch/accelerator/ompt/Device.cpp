// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

#include <omp.h>

namespace {
  int numberOfDevices = 0;
} // namespace

bool tarch::accelerator::Device::init() {
  numberOfDevices = omp_get_num_devices();
  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
#pragma omp taskwait
  numberOfDevices = 0;
}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

#endif // defined(PEANO_WITH_GPU_BACKEND_OMPT)
