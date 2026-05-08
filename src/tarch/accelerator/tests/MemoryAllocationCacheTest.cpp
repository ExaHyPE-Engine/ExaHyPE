// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryAllocationCacheTest.h"

#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

#include <type_traits>

namespace {
  constexpr int         DeviceId  = tarch::accelerator::Device::DefaultDevice;
  constexpr std::size_t ByteCount = 4 * sizeof(double);
} // namespace

static_assert(not std::is_default_constructible_v<tarch::accelerator::MemoryAllocationCache>);
static_assert(not std::is_copy_constructible_v<tarch::accelerator::MemoryAllocationCache>);
static_assert(not std::is_copy_assignable_v<tarch::accelerator::MemoryAllocationCache>);
static_assert(not std::is_move_constructible_v<tarch::accelerator::MemoryAllocationCache>);
static_assert(not std::is_move_assignable_v<tarch::accelerator::MemoryAllocationCache>);
static_assert(not std::is_destructible_v<tarch::accelerator::MemoryAllocationCache>);

tarch::accelerator::tests::MemoryAllocationCacheTest::MemoryAllocationCacheTest():
  TestCase("tarch::accelerator::tests::MemoryAllocationCacheTest") {}

void tarch::accelerator::tests::MemoryAllocationCacheTest::run() {
  testMethod(testReuseHeapAllocations);
  testMethod(testReuseDeviceAllocations);
  testMethod(testReusePinnedAllocations);
}

void tarch::accelerator::tests::MemoryAllocationCacheTest::testReuseHeapAllocations() {
  MemoryAllocationCache::getInstance().clear();

  void* first = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Heap, DeviceId);
  MemoryAllocationCache::getInstance().free(first, tarch::MemoryLocation::Heap, DeviceId);
  void* second = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Heap, DeviceId);

  validate(first != nullptr);
  validate(second != nullptr);
  validateEquals(first, second);

  MemoryAllocationCache::getInstance().free(second, tarch::MemoryLocation::Heap, DeviceId);
  MemoryAllocationCache::getInstance().clear();
}

void tarch::accelerator::tests::MemoryAllocationCacheTest::testReuseDeviceAllocations() {
  MemoryAllocationCache::getInstance().clear();

  void* first = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Device, DeviceId);
  MemoryAllocationCache::getInstance().free(first, tarch::MemoryLocation::Device, DeviceId);
  void* second = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Device, DeviceId);

  validate(first != nullptr);
  validate(second != nullptr);
  validateEquals(first, second);

  MemoryAllocationCache::getInstance().free(second, tarch::MemoryLocation::Device, DeviceId);
  MemoryAllocationCache::getInstance().clear();
}

void tarch::accelerator::tests::MemoryAllocationCacheTest::testReusePinnedAllocations() {
  MemoryAllocationCache::getInstance().clear();

  void* first = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Pinned, DeviceId);
  MemoryAllocationCache::getInstance().free(first, tarch::MemoryLocation::Pinned, DeviceId);
  void* second = MemoryAllocationCache::getInstance().allocate(ByteCount, tarch::MemoryLocation::Pinned, DeviceId);

  validate(first != nullptr);
  validate(second != nullptr);
  validateEquals(first, second);

  MemoryAllocationCache::getInstance().free(second, tarch::MemoryLocation::Pinned, DeviceId);
  MemoryAllocationCache::getInstance().clear();
}
