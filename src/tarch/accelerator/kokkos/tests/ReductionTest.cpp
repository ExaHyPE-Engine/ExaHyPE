// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

#include <Kokkos_Core.hpp>

namespace {
  constexpr int DeviceId     = tarch::accelerator::Device::DefaultDevice;
  constexpr int ElementCount = 1024;
} // namespace

tarch::accelerator::kokkos::tests::ReductionTest::ReductionTest():
  TestCase("tarch::accelerator::kokkos::tests::ReductionTest") {}

void tarch::accelerator::kokkos::tests::ReductionTest::run() { testMethod(testParallelReduction); }

void tarch::accelerator::kokkos::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;

  // tarch allocates Device memory in Kokkos::DefaultExecutionSpace::memory_space,
  // so the raw pointer can be read directly in a device parallel_reduce.
  int* deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, DeviceId);
  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, DeviceId);

  int deviceSum = 0;
  Kokkos::parallel_reduce(
    "tarch::accelerator::kokkos::tests::ReductionTest",
    Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, ElementCount),
    KOKKOS_LAMBDA(const int index, int& threadSum) { threadSum += deviceInput[index]; },
    deviceSum
  );
  Kokkos::fence();

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, DeviceId);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
