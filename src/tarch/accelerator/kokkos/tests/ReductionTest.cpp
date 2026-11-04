// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

#include <Kokkos_Core.hpp>

namespace {
  constexpr int ElementCount = 1024;

  struct AddEntry final {
    const int* values{nullptr};

    KOKKOS_FUNCTION void operator()(const int index, int& threadSum) const { threadSum += values[index]; }
  };
} // namespace

tarch::accelerator::kokkos::tests::ReductionTest::ReductionTest() :
  tarch::tests::TestCase("tarch::accelerator::kokkos::tests::ReductionTest") {}

void
tarch::accelerator::kokkos::tests::ReductionTest::run() {
  testMethod(testParallelReduction);
}

void
tarch::accelerator::kokkos::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;

  // allocateMemory() places device memory in Kokkos::DefaultExecutionSpace::memory_space. A device
  // kernel reads the raw pointer.
  int* deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device);
  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device);

  int deviceSum = 0;
  Kokkos::parallel_reduce(
    "tarch::accelerator::kokkos::tests::ReductionTest",
    Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, ElementCount),
    AddEntry{deviceInput},
    deviceSum
  );
  Kokkos::fence();

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
