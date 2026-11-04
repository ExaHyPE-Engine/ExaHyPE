// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"

#include <Kokkos_Core.hpp>

namespace {
  constexpr int ElementCount = 1024;

  struct AddEntry final {
    const int* values{nullptr};

    KOKKOS_FUNCTION void operator()(const int index, int& threadSum) const { threadSum += values[index]; }
  };
} // namespace

UNIT_TEST("A Kokkos reduction sums every element of a device array") {
  GIVEN("A device array of 1024 ones") {
    const std::vector<int> host(ElementCount, 1);

    // allocateMemory() places device memory in Kokkos::DefaultExecutionSpace::memory_space. A device
    // kernel reads the raw pointer.
    int* const deviceInput = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device);
    tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction over the default execution space sums the array") {
      int deviceSum = 0;
      Kokkos::parallel_reduce(
        "tarch/accelerator/kokkos/ReductionTest",
        Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, ElementCount),
        AddEntry{deviceInput},
        deviceSum
      );
      Kokkos::fence();

      THEN("The sum is 1024") { VALIDATE_EQUALS(deviceSum, ElementCount); }
    }
    tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device);
  }
}
