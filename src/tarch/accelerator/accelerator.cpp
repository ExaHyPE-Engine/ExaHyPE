// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "MemoryBlocks.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator");
} // namespace

#if !defined(WITH_GPU)

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, [[maybe_unused]] MemoryLocation location, int) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  // Aligned alloc requires size to be a multiple of the alignment
  void* data = std::aligned_alloc(ALIGNMENT, padSizeToAlignment(sizeInBytes, ALIGNMENT));
  assertion(data);

  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);

  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation, int) {
  if (!data) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  std::free(data);
}

void tarch::accelerator::
  copyTo(void* target, const void* source, std::size_t sizeInBytes, [[maybe_unused]] MemoryLocation location, int) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::
  copyFrom(const void* source, void* target, std::size_t sizeInBytes, [[maybe_unused]] MemoryLocation location, int) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
  std::memcpy(target, source, sizeInBytes);
}

#endif // WITH_GPU

void* tarch::accelerator::reallocateRawData(void* data, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(data);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  const std::size_t currentSizeInBytes = MemoryBlocks::getInstance().getSizeOfMemoryBlock(data);
  if (currentSizeInBytes >= sizeInBytes) {
    return data;
  }

  std::size_t newSizeInBytes = sizeInBytes + sizeInBytes / 2;
  void*       newData        = allocateRawData(newSizeInBytes, location, device);
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
