// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "MemoryAllocationCache.h"
#include "MemoryBlocks.h"
#include "tarch/Assertions.h"

#include <cstring>

#ifdef USE_MIMALLOC
#include <mimalloc.h>
#endif

void* tarch::accelerator::allocateHeapMemory(std::size_t sizeInBytes) {
#ifdef USE_MIMALLOC
  return mi_malloc_aligned(sizeInBytes, ALIGNMENT);
#else
  return ::operator new(sizeInBytes, std::align_val_t(ALIGNMENT));
#endif
}

void tarch::accelerator::freeHeapMemory(void* data) {
#ifdef USE_MIMALLOC
  mi_free(data);
#else
  ::operator delete(data, std::align_val_t(ALIGNMENT));
#endif
}

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
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

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
  if (!data) {
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

void tarch::accelerator::
  copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

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

void tarch::accelerator::
  copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion2(target, sizeInBytes, toString(location));
  assertion2(source, sizeInBytes, toString(location));
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

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

#if !defined(WITH_GPU)
void* tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  return allocateHeapMemory(sizeInBytes);
}

void* tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  return allocateHeapMemory(sizeInBytes);
}

void tarch::accelerator::freeDeviceMemory(void* data, [[maybe_unused]] int device) { freeHeapMemory(data); }

void tarch::accelerator::freePinnedMemory(void* data, [[maybe_unused]] int device) { freeHeapMemory(data); }

void tarch::accelerator::
  copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::
  copyToPinnedMemory(void* target, const void* source, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  copyToHeapMemory(target, source, sizeInBytes);
}

void tarch::accelerator::
  copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  copyFromHeapMemory(source, target, sizeInBytes);
}

void tarch::accelerator::
  copyFromPinnedMemory(const void* source, void* target, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  copyFromHeapMemory(source, target, sizeInBytes);
}
#endif // WITH_GPU
