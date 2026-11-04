// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/sycl/Device.h"
#include "tarch/tests/TestMacros.h"

#include <sycl/sycl.hpp>

namespace {
  constexpr int ElementCount = 1024;
} // namespace

tarch::accelerator::sycl::tests::ReductionTest::ReductionTest() :
  tarch::tests::TestCase("tarch::accelerator::sycl::tests::ReductionTest") {}

void
tarch::accelerator::sycl::tests::ReductionTest::run() {
  testMethod(testParallelReduction);
}

void
tarch::accelerator::sycl::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;
  const int              zero        = 0;
  const int              device      = tarch::accelerator::getCurrentDevice();

  ::sycl::queue& queue = *static_cast<::sycl::queue*>(tarch::accelerator::getSYCLQueue(device));

  int* deviceInput  = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
  int* deviceResult = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, device);

  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);
  tarch::copyTo(deviceResult, &zero, 1, tarch::MemoryLocation::Device, device);

  queue
    .submit([&](::sycl::handler& handle) {
      auto sumReduction = ::sycl::reduction(deviceResult, ::sycl::plus<int>());
      handle.parallel_for(::sycl::range<1>(ElementCount), sumReduction, [=](::sycl::id<1> index, auto& sum) {
        sum += deviceInput[index];
      });
    })
    .wait();

  int deviceSum = -1;
  tarch::copyFrom(deviceResult, &deviceSum, 1, tarch::MemoryLocation::Device, device);

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);
  tarch::freeMemory(deviceResult, tarch::MemoryLocation::Device, device);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
