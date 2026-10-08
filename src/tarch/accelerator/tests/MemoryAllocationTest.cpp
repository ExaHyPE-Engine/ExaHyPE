// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryAllocationTest.h"

#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr std::size_t                          ElementCount = 4;
  constexpr std::size_t                          ByteCount    = ElementCount * sizeof(int);
  constexpr std::array<tarch::MemoryLocation, 3> MemoryLocations{
    tarch::MemoryLocation::Heap,
    tarch::MemoryLocation::Device,
    tarch::MemoryLocation::Pinned
  };
} // namespace

tarch::accelerator::tests::MemoryAllocationTest::MemoryAllocationTest() :
  tarch::tests::TestCase("tarch::accelerator::tests::MemoryAllocationTest") {}

void
tarch::accelerator::tests::MemoryAllocationTest::run() {
  testMethod(testAllocateRawData);
  testMethod(testAllocateMemory);
  testMethod(testLocationSpecificAllocators);
  testMethod(testLocationSpecificCopyFunctions);
  testMethod(testTheCurrentDeviceHoldsForEveryHostThread);
  testMethod(testOnlyTheDeviceLocationOfAGPUBuildIsGPUMemory);
}

void
tarch::accelerator::tests::MemoryAllocationTest::testAllocateRawData() {
  const int device = tarch::accelerator::getCurrentDevice();

  for (const tarch::MemoryLocation location : MemoryLocations) {
    std::array<int, ElementCount> source{2, 4, 6, 8};
    std::array<int, ElementCount> target{0, 0, 0, 0};

    void* rawData = tarch::accelerator::allocateRawData(ByteCount, location, device);

    tarch::accelerator::copyTo(rawData, source.data(), ByteCount, location, device);
    tarch::accelerator::copyFrom(rawData, target.data(), ByteCount, location, device);

#if PEANO_DEBUG
    const std::size_t trackedSize = tarch::accelerator::MemoryBlocks::getInstance().getSizeOfMemoryBlock(rawData);
#endif
    const bool copiedCorrectly  = target == source;
    const bool allocationWorked = rawData != nullptr;

    tarch::accelerator::freeRawData(rawData, location, device);
    tarch::accelerator::freeRawData(nullptr, location, device);

    validateWithParams1(allocationWorked, tarch::toString(location));
    validateWithParams1(copiedCorrectly, tarch::toString(location));
#if PEANO_DEBUG
    validateWithParams2(trackedSize == ByteCount, tarch::toString(location), trackedSize);
#endif
  }
}

void
tarch::accelerator::tests::MemoryAllocationTest::testAllocateMemory() {
  for (const tarch::MemoryLocation location : MemoryLocations) {
    std::array<int, ElementCount> source{11, 13, 17, 19};
    std::array<int, ElementCount> target{0, 0, 0, 0};

    int* data = tarch::allocateMemory<int>(ElementCount, location);

    tarch::copyTo(data, source.data(), ElementCount, location);
    tarch::copyFrom(data, target.data(), ElementCount, location);

#if PEANO_DEBUG
    const std::size_t trackedSize = tarch::accelerator::MemoryBlocks::getInstance().getSizeOfMemoryBlock(data);
#endif
    const bool allocationWorked = data != nullptr;
    const bool copiedCorrectly  = target == source;

    tarch::freeMemory(data, location);
    tarch::freeMemory(nullptr, location);

    validateWithParams1(allocationWorked, tarch::toString(location));
    validateWithParams1(copiedCorrectly, tarch::toString(location));
#if PEANO_DEBUG
    validateWithParams2(trackedSize == ByteCount, tarch::toString(location), trackedSize);
#endif
  }
}

void
tarch::accelerator::tests::MemoryAllocationTest::testLocationSpecificAllocators() {
  const int device = tarch::accelerator::getCurrentDevice();

  for (const tarch::MemoryLocation location : MemoryLocations) {
    void* data = nullptr;

    switch (location) {
    case tarch::MemoryLocation::Heap:
      data = tarch::accelerator::allocateHeapMemory(ByteCount);
      break;
    case tarch::MemoryLocation::Device:
      data = tarch::accelerator::allocateDeviceMemory(ByteCount, device);
      break;
    case tarch::MemoryLocation::Pinned:
      data = tarch::accelerator::allocatePinnedMemory(ByteCount, device);
      break;
    }

    const bool allocationWorked = data != nullptr;

    if (data != nullptr) {
      switch (location) {
      case tarch::MemoryLocation::Heap:
        tarch::accelerator::freeHeapMemory(data);
        break;
      case tarch::MemoryLocation::Device:
        tarch::accelerator::freeDeviceMemory(data, device);
        break;
      case tarch::MemoryLocation::Pinned:
        tarch::accelerator::freePinnedMemory(data, device);
        break;
      }
    }

    validateWithParams1(allocationWorked, tarch::toString(location));
  }
}

void
tarch::accelerator::tests::MemoryAllocationTest::testLocationSpecificCopyFunctions() {
  const int device = tarch::accelerator::getCurrentDevice();

  for (const tarch::MemoryLocation location : MemoryLocations) {
    std::array<int, ElementCount> source{5, 10, 15, 20};
    std::array<int, ElementCount> target{0, 0, 0, 0};
    void*                         data = nullptr;

    switch (location) {
    case tarch::MemoryLocation::Heap:
      data = tarch::accelerator::allocateHeapMemory(ByteCount);
      tarch::accelerator::copyToHeapMemory(data, source.data(), ByteCount);
      tarch::accelerator::copyFromHeapMemory(data, target.data(), ByteCount);
      break;
    case tarch::MemoryLocation::Device:
      data = tarch::accelerator::allocateDeviceMemory(ByteCount, device);
      tarch::accelerator::copyToDeviceMemory(data, source.data(), ByteCount, device);
      tarch::accelerator::copyFromDeviceMemory(data, target.data(), ByteCount, device);
      break;
    case tarch::MemoryLocation::Pinned:
      data = tarch::accelerator::allocatePinnedMemory(ByteCount, device);
      tarch::accelerator::copyToPinnedMemory(data, source.data(), ByteCount, device);
      tarch::accelerator::copyFromPinnedMemory(data, target.data(), ByteCount, device);
      break;
    }

    const bool allocationWorked = data != nullptr;
    const bool copiedCorrectly  = target == source;

    if (data != nullptr) {
      switch (location) {
      case tarch::MemoryLocation::Heap:
        tarch::accelerator::freeHeapMemory(data);
        break;
      case tarch::MemoryLocation::Device:
        tarch::accelerator::freeDeviceMemory(data, device);
        break;
      case tarch::MemoryLocation::Pinned:
        tarch::accelerator::freePinnedMemory(data, device);
        break;
      }
    }

    validateWithParams1(allocationWorked, tarch::toString(location));
    validateWithParams1(copiedCorrectly, tarch::toString(location));
  }
}

/**
 * The device that one host thread sets is the current device of every other host thread.
 */
void
tarch::accelerator::tests::MemoryAllocationTest::testTheCurrentDeviceHoldsForEveryHostThread() {
  const int deviceOfTheRank = tarch::accelerator::getCurrentDevice();
  const int selectedDevice  = deviceOfTheRank + 1;
  int       deviceOfAnotherThread{-1};

  tarch::accelerator::setCurrentDevice(selectedDevice);
  std::thread([&deviceOfAnotherThread]() { deviceOfAnotherThread = tarch::accelerator::getCurrentDevice(); }).join();
  tarch::accelerator::setCurrentDevice(deviceOfTheRank);

  validateEquals(deviceOfAnotherThread, selectedDevice);
  validateEquals(tarch::accelerator::getCurrentDevice(), deviceOfTheRank);
}

/**
 * The heap and pinned memory are never GPU memory. The device location is GPU memory exactly where
 * its bytes count as device bytes.
 */
void
tarch::accelerator::tests::MemoryAllocationTest::testOnlyTheDeviceLocationOfAGPUBuildIsGPUMemory() {
  validate(not tarch::isGPUMemory(tarch::MemoryLocation::Heap));
  validate(not tarch::isGPUMemory(tarch::MemoryLocation::Pinned));
  validateEquals(
    tarch::isGPUMemory(tarch::MemoryLocation::Device),
    tarch::AllocatedBytes::in(tarch::MemoryLocation::Device, 1).device == 1
  );
}
