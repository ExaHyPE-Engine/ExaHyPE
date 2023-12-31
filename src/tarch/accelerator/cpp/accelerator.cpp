// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/memory/memory.h"

#if defined(GPUOffloadingCPP)

GPUCallableMethod void tarch::accelerator::gpuAbort() { std::abort(); };


void* tarch::accelerator::allocateRawData(std::size_t size, memory::MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
    case MemoryLocation::Heap:
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    case MemoryLocation::PinnedMemory:
      result = std::aligned_alloc(AlignmentOnHeap, tarch::padSizeToAlignment(size, AlignmentOnHeap));
      break;
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, memory::MemoryLocation location, int device) {
  assertion(data);

  switch (location) {
  case memory::MemoryLocation::Heap:
  case memory::MemoryLocation::UnifiedSharedMemory:
  case memory::MemoryLocation::PinnedMemory:
    std::free(data);
    break;
  }
}

#endif
