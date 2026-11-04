// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/sycl/Device.h"
#include "tarch/testing/UnitTest.h"

#include <sycl/sycl.hpp>

namespace {
  constexpr int ElementCount = 1024;
} // namespace

UNIT_TEST("A SYCL reduction sums every element of a device array into one device integer") {
  GIVEN("A device array of 1024 ones and a device result of zero on the current device") {
    const std::vector<int> host(ElementCount, 1);
    const int              zero   = 0;
    const int              device = tarch::accelerator::getCurrentDevice();

    sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(device));

    int* const deviceInput  = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
    int* const deviceResult = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, device);

    tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);
    tarch::copyTo(deviceResult, &zero, 1, tarch::MemoryLocation::Device, device);

    WHEN("A parallel loop with a sum reduction adds the array to the result and the host copies the result") {
      queue
        .submit([&](sycl::handler& handle) {
          const auto sumReduction = sycl::reduction(deviceResult, sycl::plus<int>());
          handle.parallel_for(sycl::range<1>(ElementCount), sumReduction, [=](const sycl::id<1> index, auto& sum) {
            sum += deviceInput[index];
          });
        })
        .wait();

      int deviceSum = -1;
      tarch::copyFrom(deviceResult, &deviceSum, 1, tarch::MemoryLocation::Device, device);

      THEN("The sum is 1024") { VALIDATE_EQUALS(deviceSum, ElementCount); }
    }
    tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);
    tarch::freeMemory(deviceResult, tarch::MemoryLocation::Device, device);
  }
}
