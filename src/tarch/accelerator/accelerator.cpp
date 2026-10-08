// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Warning.h"

namespace {
  std::atomic<bool> loggedMissingTaskloopSupport{false};

  /**
   * GrowingAllocation holds a memory block that grows when a request exceeds its capacity. The
   * block lives until releaseReductionScratch(). The lock of lockReductionScratch() covers both
   * blocks.
   */
  struct GrowingAllocation final {
    tarch::MemoryLocation location{tarch::MemoryLocation::Device};
    void*                 data{nullptr};
    std::size_t           capacity{0};
    int                   device{tarch::accelerator::Device::DefaultDevice};
  };

  GrowingAllocation reductionScratch{tarch::MemoryLocation::Device};
  GrowingAllocation reductionStaging{tarch::MemoryLocation::Pinned};
  std::atomic<bool> reductionScratchLocked{false};

  void release(GrowingAllocation& allocation) {
    if (allocation.data != nullptr) {
      tarch::freeMemory(allocation.data, allocation.location, allocation.device);
    }
    allocation.data     = nullptr;
    allocation.capacity = 0;
  }

  /**
   * Returns the address of the block with at least sizeInBytes bytes on the current device. A
   * larger request or another current device frees the old block and allocates a new one. The old
   * address is then invalid.
   */
  void* grow(GrowingAllocation& allocation, const std::size_t sizeInBytes) {
    assertion(sizeInBytes > 0, sizeInBytes);
    const int device = tarch::accelerator::getCurrentDevice();
    if (sizeInBytes > allocation.capacity or device != allocation.device) {
      release(allocation);
      allocation.device   = device;
      allocation.data     = tarch::allocateMemory<char>(sizeInBytes, allocation.location, device);
      allocation.capacity = sizeInBytes;
    }
    return allocation.data;
  }
} // namespace

/**
 * Returns device memory of at least sizeInBytes bytes for the partial values of a reduction and
 * locks it until unlockReductionScratch(). One reduction at a time holds the lock. A second host
 * thread that starts a reduction while the first one runs fails the assertion. The memory lives
 * until releaseReductionScratch().
 */
void*
tarch::accelerator::lockReductionScratch(const std::size_t sizeInBytes) {
  [[maybe_unused]] const bool wasLocked = reductionScratchLocked.exchange(true, std::memory_order_acquire);
  assertion(not wasLocked, sizeInBytes);
  return grow(reductionScratch, sizeInBytes);
}

/**
 * Returns pinned host memory of at least sizeInBytes bytes that receives the partial values of the
 * reduction that holds the lock.
 */
void*
tarch::accelerator::getReductionStaging(const std::size_t sizeInBytes) {
  assertion(reductionScratchLocked.load(std::memory_order_relaxed), sizeInBytes);
  return grow(reductionStaging, sizeInBytes);
}

/**
 * Releases the lock of lockReductionScratch() once the reduction has read its partial values.
 */
void
tarch::accelerator::unlockReductionScratch() {
  [[maybe_unused]] const bool wasLocked = reductionScratchLocked.exchange(false, std::memory_order_release);
  assertion(wasLocked);
}

/**
 * Frees the memory of both reduction buffers. The device shutdown calls this before it destroys
 * the device context. No reduction holds the lock then.
 */
void
tarch::accelerator::releaseReductionScratch() {
  assertion(not reductionScratchLocked.load(std::memory_order_relaxed));
  release(reductionScratch);
  release(reductionStaging);
}

/**
 * Returns the number of tasks that split a host loop over numberOfItems items of secondsPerItem
 * seconds each into tasks of MinimumSecondsPerHostTask seconds. The number lies between one task
 * per thread and MaximumHostTasksPerThread tasks per thread. It exceeds numberOfItems only for an
 * empty loop.
 */
int
tarch::accelerator::getNumberOfHostTasks(const int numberOfItems, const int numberOfThreads, const double secondsPerItem) {
  assertion(numberOfItems >= 0, numberOfItems);
  assertion(numberOfThreads > 0, numberOfThreads);
  assertion(std::isfinite(secondsPerItem) and secondsPerItem >= 0.0, secondsPerItem);
  const int    fewestTasks        = std::max(1, std::min(numberOfItems, numberOfThreads));
  const int    mostTasks          = std::max(1, std::min(numberOfItems, MaximumHostTasksPerThread * numberOfThreads));
  const double tasksOfMinimumWork = std::min(
    numberOfItems * secondsPerItem / MinimumSecondsPerHostTask,
    static_cast<double>(mostTasks)
  );
  return std::clamp(static_cast<int>(std::lround(tasksOfMinimumWork)), fewestTasks, mostTasks);
}

void
tarch::accelerator::reportMissingTaskloopSupportOnce() {
  if (not loggedMissingTaskloopSupport.exchange(true, std::memory_order_relaxed)) {
    logWarning("The compiler does not support OpenMP taskloops. The parallel loops run slower.");
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
 * Sets the device of every kernel launch and of every allocation without an explicit device, for
 * all host threads. tarch::init() sets the device of the rank before the first allocation.
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

void
tarch::accelerator::waitForDevice() {}

#endif // PEANO_WITH_GPU_BACKEND
