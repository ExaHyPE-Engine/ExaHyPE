// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
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
void
tarch::accelerator::reportMissingTaskloopSupportOnce() {
  if (not loggedMissingTaskloopSupport.exchange(true, std::memory_order_relaxed)) {
    logWarning(
      "OpenMP taskloop is unavailable on this toolchain, so a parallel range spawns one task per chunk. Expect "
      "degraded performance."
    );
  }
}

void*
tarch::accelerator::allocateHeapMemory(const std::size_t sizeInBytes) {
  return ::operator new(sizeInBytes, std::align_val_t(PEANO_WITH_ALIGNMENT));
}

void
tarch::accelerator::freeHeapMemory(void* const data) {
  ::operator delete(data, std::align_val_t(PEANO_WITH_ALIGNMENT));
}

void
tarch::accelerator::copyToHeapMemory(void* const target, const void* const source, const std::size_t sizeInBytes) {
  std::memcpy(target, source, sizeInBytes);
}

void
tarch::accelerator::copyFromHeapMemory(const void* const source, void* const target, const std::size_t sizeInBytes) {
  std::memcpy(target, source, sizeInBytes);
}

void
tarch::freeMemory(void* const memoryPtr, const MemoryLocation location, const int device) {
  accelerator::freeRawData(memoryPtr, location, device);
}

void*
tarch::accelerator::allocateRawData(const std::size_t sizeInBytes, const MemoryLocation location, const int device) {
  assertion(sizeInBytes > 0, sizeInBytes, toString(location));

  void* data = nullptr;
  switch (location) {
  case MemoryLocation::Heap:
    data = allocateHeapMemory(sizeInBytes);
    break;
  case MemoryLocation::Pinned:
    data = allocatePinnedMemory(sizeInBytes, device);
    break;
  case MemoryLocation::Device:
    data = allocateDeviceMemory(sizeInBytes, device);
    break;
  }
  assertion(data);

  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);
  return data;
}

void
tarch::accelerator::freeRawData(void* const data, const MemoryLocation location, const int device) {
  if (data == nullptr) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  switch (location) {
  case MemoryLocation::Heap:
    freeHeapMemory(data);
    break;
  case MemoryLocation::Pinned:
    freePinnedMemory(data, device);
    break;
  case MemoryLocation::Device:
    freeDeviceMemory(data, device);
    break;
  }
}

void
tarch::accelerator::copyTo(
  void* const          target,
  const void* const    source,
  const std::size_t    sizeInBytes,
  const MemoryLocation location,
  const int            device
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

void
tarch::accelerator::copyFrom(
  const void* const    source,
  void* const          target,
  const std::size_t    sizeInBytes,
  const MemoryLocation location,
  const int            device
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
  int currentDevice = tarch::accelerator::Device::DefaultDevice;
} // namespace

/**
 * Selects the device of every kernel launch and of every allocation that names no device, for
 * every host thread. tarch::init() selects the device of the rank before the first allocation.
 */
void
tarch::accelerator::setCurrentDevice(const int device) {
  assertion(device >= 0, device);
  currentDevice = device;
}

int
tarch::accelerator::getCurrentDevice() {
  return currentDevice;
}

#if not defined(PEANO_WITH_GPU_BACKEND)
void*
tarch::accelerator::allocateDeviceMemory(const std::size_t sizeInBytes, [[maybe_unused]] const int device) {
  return allocateHeapMemory(sizeInBytes);
}

void*
tarch::accelerator::allocatePinnedMemory(const std::size_t sizeInBytes, [[maybe_unused]] const int device) {
  return allocateHeapMemory(sizeInBytes);
}

void
tarch::accelerator::freeDeviceMemory(void* const data, [[maybe_unused]] const int device) {
  freeHeapMemory(data);
}

void
tarch::accelerator::freePinnedMemory(void* const data, [[maybe_unused]] const int device) {
  freeHeapMemory(data);
}

void
tarch::accelerator::copyToDeviceMemory(
  void* const                target,
  const void* const          source,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void
tarch::accelerator::copyToPinnedMemory(
  void* const                target,
  const void* const          source,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void
tarch::accelerator::copyFromDeviceMemory(
  const void* const          source,
  void* const                target,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}

void
tarch::accelerator::copyFromPinnedMemory(
  const void* const          source,
  void* const                target,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}
#endif // PEANO_WITH_GPU_BACKEND
