// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator");
} // namespace

#if defined(GPUOffloadingOff)

void* tarch::accelerator::allocateRawData(
  std::size_t                     sizeInBytes,
  [[maybe_unused]] MemoryLocation location,
  int
) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  // Aligned alloc requires size to be a multiple of the alignment
  void* data = std::aligned_alloc(
    ALIGNMENT_ON_HEAP,
    padSizeToAlignment(sizeInBytes, ALIGNMENT_ON_HEAP)
  );
  assertion(data);

#if PeanoDebug > 0
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }
#endif

  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation, int) {
  if (!data) {
    return;
  }

#if PeanoDebug > 0
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logDebug("freeRawData()", "Attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }
#endif

  std::free(data);
}

void tarch::accelerator::copyTo(
  void*                           target,
  const void*                     source,
  std::size_t                     sizeInBytes,
  [[maybe_unused]] MemoryLocation location,
  int
) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFrom(
  const void*                     source,
  void*                           target,
  std::size_t                     sizeInBytes,
  [[maybe_unused]] MemoryLocation location,
  int
) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
  std::memcpy(target, source, sizeInBytes);
}

#endif // defined(GPUOffloadingOff)

void* tarch::accelerator::reallocateRawData(
  void*          data,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(data);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

#if PeanoDebug > 0
  std::size_t currentSizeInBytes = 0;
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    currentSizeInBytes = Device::getInstance().getMemoryBlockTable().at(data);
  }
  if (currentSizeInBytes >= sizeInBytes) {
    return data;
  }
#endif

  std::size_t newSizeInBytes = sizeInBytes + sizeInBytes / 2;
  void* newData = allocateRawData(newSizeInBytes, location, device);
  assertion(newData);

  // TODO: Copy host to host
  freeRawData(data, location, device);

  return newData;
}

void tarch::freeMemory(void* memoryPtr, MemoryLocation location, int device) {
  accelerator::freeRawData(memoryPtr, location, device);
}

std::size_t tarch::padSizeToAlignment(std::size_t size, std::size_t alignment) {
  assertion1(size > 0, size);
  assertion1(alignment > 0, alignment);
  // Aligned alloc requires size to be a multiple of alignment, therefore we
  // need to check it. Also see:
  // https://en.cppreference.com/w/c/memory/aligned_alloc
  // Pad to nearest multiple of alignment.
  int padding = size % alignment == 0 ? 0 : alignment - (size % alignment);
  return size + padding;
}
