// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "MemoryBlocks.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <cstring>

#ifdef USE_MIMALLOC
#include <mimalloc.h>
#endif

namespace {
  tarch::logging::Log _log("tarch::accelerator");
} // namespace

void* tarch::accelerator::allocateRawDataOnHeap(std::size_t sizeInBytes) {
#ifdef USE_MIMALLOC
  return mi_malloc_aligned(sizeInBytes, ALIGNMENT);
#else
  return ::operator new(sizeInBytes, std::align_val_t(ALIGNMENT));
#endif
}

void tarch::accelerator::freeRawDataOnHeap(void* data) {
#ifdef USE_MIMALLOC
  mi_free(data);
#else
  ::operator delete(data, std::align_val_t(ALIGNMENT));
#endif
}

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

#if !defined(WITH_GPU)
void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, [[maybe_unused]] MemoryLocation location, int) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  void* data = allocateRawDataOnHeap(sizeInBytes);
  assertion(data);

  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);

  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation, int) {
  if (!data) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  freeRawDataOnHeap(data);
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
