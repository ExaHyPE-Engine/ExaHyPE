// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Warning.h"

namespace {
  std::atomic<bool> loggedMissingTaskloopSupport{false};
} // namespace

/**
 * Writes one warning per run that this toolchain offers no OpenMP taskloop. parallelFor() is a
 * template, so its fallback branch reaches this function from every translation unit that uses it.
 */
void tarch::accelerator::reportMissingTaskloopSupportOnce() {
  if (not loggedMissingTaskloopSupport.exchange(true, std::memory_order_relaxed)) {
    logWarning(
      "OpenMP taskloop is unavailable on this toolchain. A parallel range is split into tasks by hand, which costs one spawn per chunk. Expect degraded performance."
    );
  }
}

void* tarch::accelerator::allocateHeapMemory(std::size_t sizeInBytes) {
  return ::operator new(sizeInBytes, std::align_val_t(PEANO_WITH_ALIGNMENT));
}

void tarch::accelerator::freeHeapMemory(void* data) { ::operator delete(data, std::align_val_t(PEANO_WITH_ALIGNMENT)); }

void tarch::accelerator::copyToHeapMemory(void* target, const void* source, std::size_t sizeInBytes) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromHeapMemory(const void* source, void* target, std::size_t sizeInBytes) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::freeMemory(void* memoryPtr, MemoryLocation location, int device) {
  accelerator::freeRawData(memoryPtr, location, device);
}

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(sizeInBytes > 0, sizeInBytes, toString(location));

  void* data = nullptr;
  switch (location) {
  case MemoryLocation::Heap:
    data = MemoryAllocationCache::getInstance().allocate(sizeInBytes, location, device);
    break;
  case MemoryLocation::Pinned:
    data = MemoryAllocationCache::getInstance().allocate(sizeInBytes, location, device);
    break;
  case MemoryLocation::Device:
    data = MemoryAllocationCache::getInstance().allocate(sizeInBytes, location, device);
    break;
  }
  assertion(data);

  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);
  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation location, int device) {
  if (data == nullptr) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  switch (location) {
  case MemoryLocation::Heap:
    MemoryAllocationCache::getInstance().free(data, location, device);
    break;
  case MemoryLocation::Pinned:
    MemoryAllocationCache::getInstance().free(data, location, device);
    break;
  case MemoryLocation::Device:
    MemoryAllocationCache::getInstance().free(data, location, device);
    break;
  }
}

void tarch::accelerator::copyTo(
  void*          target,
  const void*    source,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(target, sizeInBytes, toString(location));
  assertion(source, sizeInBytes, toString(location));
  assertion(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("Copy " << sizeInBytes << " bytes from host to host");
    copyToHeapMemory(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
    logDebug("Copy " << sizeInBytes << " bytes from host to device");
    copyToDeviceMemory(target, source, sizeInBytes, device);
    break;
  case MemoryLocation::Pinned:
    logDebug("Copy " << sizeInBytes << " bytes from host to pinned host memory");
    copyToPinnedMemory(target, source, sizeInBytes, device);
    break;
  }
}

void tarch::accelerator::copyFrom(
  const void*    source,
  void*          target,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(target, sizeInBytes, toString(location));
  assertion(source, sizeInBytes, toString(location));
  assertion(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("Copy " << sizeInBytes << " bytes from host to host");
    copyFromHeapMemory(source, target, sizeInBytes);
    break;
  case MemoryLocation::Device:
    logDebug("Copy " << sizeInBytes << " bytes from device to host");
    copyFromDeviceMemory(source, target, sizeInBytes, device);
    break;
  case MemoryLocation::Pinned:
    logDebug("Copy " << sizeInBytes << " bytes from pinned host memory to host");
    copyFromPinnedMemory(source, target, sizeInBytes, device);
    break;
  }
}

namespace {
  thread_local int currentDevice = tarch::accelerator::Device::DefaultDevice;
} // namespace

/**
 * Selects the device that tarch::parallelFor and tarch::parallelReduce target. Every host thread
 * holds its own selection, which starts at Device::DefaultDevice.
 */
void tarch::accelerator::setCurrentDevice(int device) { currentDevice = device; }

int tarch::accelerator::getCurrentDevice() { return currentDevice; }

#if not defined(PEANO_WITH_GPU_BACKEND)
void* tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  return allocateHeapMemory(sizeInBytes);
}

void* tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  return allocateHeapMemory(sizeInBytes);
}

void tarch::accelerator::freeDeviceMemory(void* data, [[maybe_unused]] int device) { freeHeapMemory(data); }

void tarch::accelerator::freePinnedMemory(void* data, [[maybe_unused]] int device) { freeHeapMemory(data); }

void tarch::accelerator::copyToDeviceMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::copyToPinnedMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromDeviceMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}

void tarch::accelerator::copyFromPinnedMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}
#endif // PEANO_WITH_GPU_BACKEND
