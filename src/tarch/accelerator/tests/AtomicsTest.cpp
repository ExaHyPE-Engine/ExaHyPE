// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AtomicsTest.h"

#include "tarch/accelerator/Atomics.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  constexpr int NumberOfValues = 64;
  constexpr int DeviceId       = tarch::accelerator::Device::DefaultDevice;
} // namespace

tarch::accelerator::tests::AtomicsTest::AtomicsTest() :
  tarch::tests::TestCase("tarch::accelerator::tests::AtomicsTest") {}

void tarch::accelerator::tests::AtomicsTest::run() {
  testMethod(testRaiseKeepsTheMaximum);
  testMethod(testLoadStoreRoundTrip);
  testMethod(testConcurrentMaximumIsOrderIndependent);
}

void tarch::accelerator::tests::AtomicsTest::testRaiseKeepsTheMaximum() {
  int value = 5;
  tarch::accelerator::atomicMaximum(&value, 3);
  validateEquals(value, 5);
  tarch::accelerator::atomicMaximum(&value, 9);
  validateEquals(value, 9);
}

void tarch::accelerator::tests::AtomicsTest::testLoadStoreRoundTrip() {
  int value = 0;
  tarch::accelerator::atomicStore(&value, 7);
  validateEquals(tarch::accelerator::atomicLoad(&value), 7);
}

void tarch::accelerator::tests::AtomicsTest::testConcurrentMaximumIsOrderIndependent() {
  int* const target = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, DeviceId);

  int hostMaximum = std::numeric_limits<int>::lowest();
  tarch::copyTo(target, &hostMaximum, 1, tarch::MemoryLocation::Device, DeviceId);

  tarch::parallelFor(tarch::Range(NumberOfValues), [=] GPU_CALLABLE_LAMBDA(const int value) {
    tarch::accelerator::atomicMaximum(target, value);
  });

  tarch::copyFrom(target, &hostMaximum, 1, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(target, tarch::MemoryLocation::Device, DeviceId);
  validateEquals(hostMaximum, NumberOfValues - 1);
}
