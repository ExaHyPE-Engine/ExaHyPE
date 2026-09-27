// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int ElementCount = 1024;
} // namespace

tarch::accelerator::ompt::tests::ReductionTest::ReductionTest() :
  tarch::tests::TestCase("tarch::accelerator::ompt::tests::ReductionTest") {}

void
tarch::accelerator::ompt::tests::ReductionTest::run() {
  testMethod(testParallelReduction);
}

void
tarch::accelerator::ompt::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;
  const int              device      = tarch::accelerator::getCurrentDevice();

  // The allocation of tarch runs through omp_target_alloc, so the pointer is a device pointer and
  // the target region takes it with is_device_ptr.
  int* deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);

  int deviceSum = 0;
#pragma omp target teams distribute parallel for reduction(+ : deviceSum) device(device) is_device_ptr(deviceInput)
  for (int index = 0; index < ElementCount; ++index) {
    deviceSum += deviceInput[index];
  }

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
