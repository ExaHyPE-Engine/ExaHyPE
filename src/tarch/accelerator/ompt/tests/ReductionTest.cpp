// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

#include <vector>

namespace {
  constexpr int DeviceId     = tarch::accelerator::Device::DefaultDevice;
  constexpr int ElementCount = 1024;
} // namespace

tarch::accelerator::ompt::tests::ReductionTest::ReductionTest():
  TestCase("tarch::accelerator::ompt::tests::ReductionTest") {}

void tarch::accelerator::ompt::tests::ReductionTest::run() { testMethod(testParallelReduction); }

void tarch::accelerator::ompt::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;

  // tarch allocates Device memory via omp_target_alloc, so the pointer is already
  // a device pointer and is passed in with is_device_ptr.
  int* deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, DeviceId);
  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, DeviceId);

  int deviceSum = 0;
#pragma omp target teams distribute parallel for reduction(+ : deviceSum) device(DeviceId) is_device_ptr(deviceInput)
  for (int index = 0; index < ElementCount; ++index) {
    deviceSum += deviceInput[index];
  }

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, DeviceId);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
