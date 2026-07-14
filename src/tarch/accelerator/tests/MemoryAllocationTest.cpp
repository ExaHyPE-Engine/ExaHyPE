// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryAllocationTest.h"

#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int         DeviceId     = tarch::accelerator::Device::DefaultDevice;
  constexpr std::size_t ElementCount = 4;
  constexpr std::size_t ByteCount    = ElementCount * sizeof(int);
  constexpr std::array<tarch::MemoryLocation, 3>
    MemoryLocations{tarch::MemoryLocation::Heap, tarch::MemoryLocation::Device, tarch::MemoryLocation::Pinned};
} // namespace

tarch::accelerator::tests::MemoryAllocationTest::MemoryAllocationTest():
  TestCase("tarch::accelerator::tests::MemoryAllocationTest") {}

void tarch::accelerator::tests::MemoryAllocationTest::run() {
  testMethod(testAllocateRawData);
  testMethod(testAllocateMemory);
  testMethod(testLocationSpecificAllocators);
  testMethod(testLocationSpecificCopyFunctions);
}

void tarch::accelerator::tests::MemoryAllocationTest::testAllocateRawData() {
  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();

  for (tarch::MemoryLocation location : MemoryLocations) {
    std::array<int, ElementCount> source{2, 4, 6, 8};
    std::array<int, ElementCount> target{0, 0, 0, 0};

    void* rawData = tarch::accelerator::allocateRawData(ByteCount, location, DeviceId);

    tarch::accelerator::copyTo(rawData, source.data(), ByteCount, location, DeviceId);
    tarch::accelerator::copyFrom(rawData, target.data(), ByteCount, location, DeviceId);

#if PEANO_DEBUG
    const std::size_t trackedSize = MemoryBlocks::getInstance().getSizeOfMemoryBlock(rawData);
#endif
    const bool copiedCorrectly  = target == source;
    const bool allocationWorked = rawData != nullptr;

    tarch::accelerator::freeRawData(rawData, location, DeviceId);
    tarch::accelerator::freeRawData(nullptr, location, DeviceId);

    validateWithParams1(allocationWorked, toString(location));
    validateWithParams1(copiedCorrectly, toString(location));
#if PEANO_DEBUG
    validateWithParams2(trackedSize == ByteCount, toString(location), trackedSize);
#endif
  }

  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();
}

void tarch::accelerator::tests::MemoryAllocationTest::testAllocateMemory() {
  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();

  for (tarch::MemoryLocation location : MemoryLocations) {
    std::array<int, ElementCount> source{11, 13, 17, 19};
    std::array<int, ElementCount> target{0, 0, 0, 0};

    int* data = tarch::allocateMemory<int>(ElementCount, location, DeviceId);

    tarch::copyTo(data, source.data(), ElementCount, location, DeviceId);
    tarch::copyFrom(data, target.data(), ElementCount, location, DeviceId);

#if PEANO_DEBUG
    const std::size_t trackedSize = MemoryBlocks::getInstance().getSizeOfMemoryBlock(data);
#endif
    const bool allocationWorked = data != nullptr;
    const bool copiedCorrectly  = target == source;

    tarch::freeMemory(data, location, DeviceId);
    tarch::freeMemory(nullptr, location, DeviceId);

    validateWithParams1(allocationWorked, toString(location));
    validateWithParams1(copiedCorrectly, toString(location));
#if PEANO_DEBUG
    validateWithParams2(trackedSize == ByteCount, toString(location), trackedSize);
#endif
  }

  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();
}

void tarch::accelerator::tests::MemoryAllocationTest::testLocationSpecificAllocators() {
  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();

  for (tarch::MemoryLocation location : MemoryLocations) {
    void* data = nullptr;

    switch (location) {
    case tarch::MemoryLocation::Heap:
      data = tarch::accelerator::allocateHeapMemory(ByteCount);
      break;
    case tarch::MemoryLocation::Device:
      data = tarch::accelerator::allocateDeviceMemory(ByteCount, DeviceId);
      break;
    case tarch::MemoryLocation::Pinned:
      data = tarch::accelerator::allocatePinnedMemory(ByteCount, DeviceId);
      break;
    }

    const bool allocationWorked = data != nullptr;

    if (data != nullptr) {
      switch (location) {
      case tarch::MemoryLocation::Heap:
        tarch::accelerator::freeHeapMemory(data);
        break;
      case tarch::MemoryLocation::Device:
        tarch::accelerator::freeDeviceMemory(data, DeviceId);
        break;
      case tarch::MemoryLocation::Pinned:
        tarch::accelerator::freePinnedMemory(data, DeviceId);
        break;
      }
    }

    validateWithParams1(allocationWorked, toString(location));
  }

  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();
}

void tarch::accelerator::tests::MemoryAllocationTest::testLocationSpecificCopyFunctions() {
  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();

  for (tarch::MemoryLocation location : MemoryLocations) {
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
      data = tarch::accelerator::allocateDeviceMemory(ByteCount, DeviceId);
      tarch::accelerator::copyToDeviceMemory(data, source.data(), ByteCount, DeviceId);
      tarch::accelerator::copyFromDeviceMemory(data, target.data(), ByteCount, DeviceId);
      break;
    case tarch::MemoryLocation::Pinned:
      data = tarch::accelerator::allocatePinnedMemory(ByteCount, DeviceId);
      tarch::accelerator::copyToPinnedMemory(data, source.data(), ByteCount, DeviceId);
      tarch::accelerator::copyFromPinnedMemory(data, target.data(), ByteCount, DeviceId);
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
        tarch::accelerator::freeDeviceMemory(data, DeviceId);
        break;
      case tarch::MemoryLocation::Pinned:
        tarch::accelerator::freePinnedMemory(data, DeviceId);
        break;
      }
    }

    validateWithParams1(allocationWorked, toString(location));
    validateWithParams1(copiedCorrectly, toString(location));
  }

  MemoryAllocationCache::getInstance().clear();
  MemoryBlocks::getInstance().clear();
}
