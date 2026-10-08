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

bool
tarch::accelerator::Device::init() {
  numberOfDevices = omp_get_num_devices();
  return isInitialised();
}

void
tarch::accelerator::Device::shutdown() {
#pragma omp taskwait
  tarch::accelerator::releaseReductionScratch();
  numberOfDevices = 0;
}

bool
tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

/**
 * Returns the threads the current device runs at once: The teams of a launch without a team
 * count times their threads. The OpenMP runtime picks that team count from the device.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  const int device  = tarch::accelerator::getCurrentDevice();
  int       teams   = 0;
  int       threads = 0;
#pragma omp target teams map(from : teams, threads) device(device)
#pragma omp parallel
  {
    if (omp_get_team_num() == 0 and omp_get_thread_num() == 0) {
      teams   = omp_get_num_teams();
      threads = omp_get_num_threads();
    }
  }
  return std::max(1, teams * threads);
}

/**
 * Returns std::nullopt. The host pass of an OpenMP target build tells no device vendor that an MPI
 * query names.
 */
std::optional<bool>
tarch::accelerator::Device::isMPIGPUAware() const {
  return std::nullopt;
}

/**
 * Returns an empty string. OpenMP has no physical address of a device.
 */
std::string
tarch::accelerator::Device::getIdOfCurrentDevice() const {
  return {};
}

#endif // defined(PEANO_WITH_GPU_BACKEND_OMPT)
