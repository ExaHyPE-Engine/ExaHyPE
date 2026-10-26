// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int ElementCount = 1024;
} // namespace

UNIT_TEST("An OpenMP target reduction sums every element of a device array") {
  GIVEN("A device array of 1024 ones on the current device") {
    const std::vector<int> host(ElementCount, 1);
    const int              device = tarch::accelerator::getCurrentDevice();

    // allocateMemory() calls omp_target_alloc(). The target region takes the device pointer with
    // is_device_ptr.
    int* const deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
    tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);

    WHEN("A target region sums the array in a reduction") {
      int deviceSum = 0;
#pragma omp target teams distribute parallel for reduction(+ : deviceSum) device(device) is_device_ptr(deviceInput)
      for (int index = 0; index < ElementCount; ++index) {
        deviceSum += deviceInput[index];
      }

      THEN("The sum is 1024") { VALIDATE_EQUALS(deviceSum, ElementCount); }
    }
    tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);
  }
}
