// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Warning.h"

#if not defined(_WIN32)
#include <sys/mman.h>
#endif

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

void* tarch::accelerator::allocateHeapMemory(const std::size_t sizeInBytes) {
  return ::operator new(sizeInBytes, std::align_val_t(PEANO_WITH_ALIGNMENT));
}

void tarch::accelerator::freeHeapMemory(void* const data) {
  ::operator delete(data, std::align_val_t(PEANO_WITH_ALIGNMENT));
}

/**
 * Maps @p sizeInBytes of zero-filled, page-aligned memory. A page of the
 * mapping turns resident on its first write. The writing thread's NUMA node
 * holds the page.
 */
void* tarch::accelerator::allocateZeroedHeapMemory(const std::size_t sizeInBytes) {
#if defined(_WIN32)
  void* const data = allocateHeapMemory(sizeInBytes);
  std::memset(data, 0, sizeInBytes);
  return data;
#else
  void* const data = ::mmap(nullptr, sizeInBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (data == MAP_FAILED) {
    tarch::terminateWithError("Mapping " + std::to_string(sizeInBytes) + " bytes of zeroed memory failed.");
  }
  return data;
#endif
}

void tarch::accelerator::freeZeroedHeapMemory(void* const data, [[maybe_unused]] const std::size_t sizeInBytes) {
#if defined(_WIN32)
  freeHeapMemory(data);
#else
  [[maybe_unused]] const int result = ::munmap(data, sizeInBytes);
  assertion(result == 0, sizeInBytes);
#endif
}

void tarch::accelerator::copyToHeapMemory(void* const target, const void* const source, const std::size_t sizeInBytes) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromHeapMemory(
  const void* const source,
  void* const       target,
  const std::size_t sizeInBytes
) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::freeMemory(void* const memoryPtr, const MemoryLocation location, const int device) {
  accelerator::freeRawData(memoryPtr, location, device);
}

void* tarch::accelerator::allocateRawData(
  const std::size_t    sizeInBytes,
  const MemoryLocation location,
  const int            device
) {
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

void tarch::accelerator::freeRawData(void* const data, const MemoryLocation location, const int device) {
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

void tarch::accelerator::copyTo(
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

void tarch::accelerator::copyFrom(
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
  thread_local int currentDevice = tarch::accelerator::Device::DefaultDevice;
} // namespace

/**
 * Selects the device that tarch::accelerator::parallelFor and tarch::accelerator::parallelReduce
 * target. Every host thread holds its own selection, which starts at Device::DefaultDevice.
 */
void tarch::accelerator::setCurrentDevice(const int device) { currentDevice = device; }

int tarch::accelerator::getCurrentDevice() { return currentDevice; }

#if not defined(PEANO_WITH_GPU_BACKEND)
void* tarch::accelerator::allocateDeviceMemory(const std::size_t sizeInBytes, [[maybe_unused]] const int device) {
  return allocateHeapMemory(sizeInBytes);
}

void* tarch::accelerator::allocatePinnedMemory(const std::size_t sizeInBytes, [[maybe_unused]] const int device) {
  return allocateHeapMemory(sizeInBytes);
}

void tarch::accelerator::freeDeviceMemory(void* const data, [[maybe_unused]] const int device) { freeHeapMemory(data); }

void tarch::accelerator::freePinnedMemory(void* const data, [[maybe_unused]] const int device) { freeHeapMemory(data); }

void tarch::accelerator::copyToDeviceMemory(
  void* const                target,
  const void* const          source,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::copyToPinnedMemory(
  void* const                target,
  const void* const          source,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromDeviceMemory(
  const void* const          source,
  void* const                target,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}

void tarch::accelerator::copyFromPinnedMemory(
  const void* const          source,
  void* const                target,
  const std::size_t          sizeInBytes,
  [[maybe_unused]] const int device
) {
  copyFromHeapMemory(source, target, sizeInBytes);
}
#endif // PEANO_WITH_GPU_BACKEND
