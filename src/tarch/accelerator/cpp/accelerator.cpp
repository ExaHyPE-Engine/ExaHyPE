// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/memory/memory.h"

#if defined(GPUOffloadingCPP)

GPUCallableMethod void tarch::accelerator::gpuAbort() { std::abort(); };


void* tarch::accelerator::allocateRawData(std::size_t size, tarch::memory::MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
    case tarch::MemoryLocation::Heap:
    case tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    case tarch::MemoryLocation::PinnedMemory:
      result = std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
      break;
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, tarch::memory::MemoryLocation location, int device) {
  assertion(data);

  switch (location) {
  case tarch::memory::MemoryLocation::Heap:
  case tarch::memory::MemoryLocation::UnifiedSharedMemory:
  case tarch::memory::MemoryLocation::PinnedMemory:
    std::free(data);
    break;
  }
}

#endif
