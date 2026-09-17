// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AtomicTest.h"

#include "tarch/accelerator/Atomic.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  constexpr int NumberOfThreads    = 8;
  constexpr int NumberOfIterations = 2048;
  constexpr int NumberOfValues     = 64;
  constexpr int DeviceId           = tarch::accelerator::Device::DefaultDevice;
} // namespace

tarch::accelerator::tests::AtomicTest::AtomicTest() :
  tarch::tests::TestCase("tarch::accelerator::tests::AtomicTest") {}

void tarch::accelerator::tests::AtomicTest::run() {
  testMethod(testSerialOperations);
  testMethod(testConcurrentOperations);
  testMethod(testLoadStoreRoundTrip);
  testMethod(testConcurrentMaximumIsOrderIndependent);
}

void tarch::accelerator::tests::AtomicTest::testSerialOperations() {
  int intValue = 10;
  tarch::accelerator::updateAtomicMin(intValue, 3);
  validateEquals(intValue, 3);
  tarch::accelerator::updateAtomicMin(intValue, 7);
  validateEquals(intValue, 3);
  tarch::accelerator::updateAtomicMax(intValue, 12);
  validateEquals(intValue, 12);
  tarch::accelerator::updateAtomicMax(intValue, 5);
  validateEquals(intValue, 12);
  validateEquals(tarch::accelerator::incrementAtomic(intValue), 13);
  validateEquals(intValue, 13);
  validateEquals(tarch::accelerator::addAtomic(intValue, 4), 17);
  validateEquals(intValue, 17);
  tarch::accelerator::storeAtomic(intValue, 2);
  validateEquals(intValue, 2);

  double doubleValue = 4.0;
  tarch::accelerator::updateAtomicMin(doubleValue, 2.5);
  validateEquals(doubleValue, 2.5);
  tarch::accelerator::updateAtomicMax(doubleValue, 5.25);
  validateEquals(doubleValue, 5.25);
  validateEquals(tarch::accelerator::incrementAtomic(doubleValue), 6.25);
  validateEquals(doubleValue, 6.25);
  validateEquals(tarch::accelerator::addAtomic(doubleValue, 1.5), 7.75);
  validateEquals(doubleValue, 7.75);
  tarch::accelerator::storeAtomic(doubleValue, -std::numeric_limits<double>::infinity());
  validateEquals(doubleValue, -std::numeric_limits<double>::infinity());
  tarch::accelerator::updateAtomicMin(doubleValue, 1.0);
  validateEquals(doubleValue, -std::numeric_limits<double>::infinity());
}

void tarch::accelerator::tests::AtomicTest::testConcurrentOperations() {
  constexpr int NumberOfUpdates = NumberOfThreads * NumberOfIterations;

  int    minInt     = std::numeric_limits<int>::max();
  int    maxInt     = std::numeric_limits<int>::min();
  int    intCounter = 0;
  int    intSum     = 0;
  double minDouble  = std::numeric_limits<double>::max();
  double maxDouble  = -std::numeric_limits<double>::max();
  double doubleSum  = 0.0;

  std::vector<std::thread> threads;
  threads.reserve(NumberOfThreads);
  for (int threadIndex = 0; threadIndex < NumberOfThreads; ++threadIndex) {
    threads.emplace_back([&, threadIndex]() {
      for (int iteration = 0; iteration < NumberOfIterations; ++iteration) {
        const int    value       = threadIndex * NumberOfIterations + iteration;
        const double doubleValue = static_cast<double>(value) + 0.25;
        tarch::accelerator::updateAtomicMin(minInt, value);
        tarch::accelerator::updateAtomicMax(maxInt, value);
        tarch::accelerator::incrementAtomic(intCounter);
        tarch::accelerator::addAtomic(intSum, 1);
        tarch::accelerator::updateAtomicMin(minDouble, doubleValue);
        tarch::accelerator::updateAtomicMax(maxDouble, doubleValue);
        tarch::accelerator::addAtomic(doubleSum, 1.0);
      }
    });
  }
  for (auto& workerThread : threads) {
    workerThread.join();
  }

  validateEquals(minInt, 0);
  validateEquals(maxInt, NumberOfUpdates - 1);
  validateEquals(intCounter, NumberOfUpdates);
  validateEquals(intSum, NumberOfUpdates);
  validateEquals(minDouble, 0.25);
  validateEquals(maxDouble, static_cast<double>(NumberOfUpdates - 1) + 0.25);
  validateEquals(doubleSum, static_cast<double>(NumberOfUpdates));
}

void tarch::accelerator::tests::AtomicTest::testLoadStoreRoundTrip() {
  int value = 0;
  tarch::accelerator::storeAtomic(value, 7);
  validateEquals(tarch::accelerator::loadAtomic(value), 7);
}

void tarch::accelerator::tests::AtomicTest::testConcurrentMaximumIsOrderIndependent() {
  int* const target = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, DeviceId);

  int hostMaximum = std::numeric_limits<int>::lowest();
  tarch::copyTo(target, &hostMaximum, 1, tarch::MemoryLocation::Device, DeviceId);

  tarch::parallelFor(tarch::Range(NumberOfValues), [=] GPU_CALLABLE_LAMBDA(const int value) {
    tarch::accelerator::updateAtomicMax(*target, value);
  });

  tarch::copyFrom(target, &hostMaximum, 1, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(target, tarch::MemoryLocation::Device, DeviceId);
  validateEquals(hostMaximum, NumberOfValues - 1);
}
