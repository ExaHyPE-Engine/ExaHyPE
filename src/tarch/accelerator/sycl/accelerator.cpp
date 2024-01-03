// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/memory/memory.h"

#if defined(GPUOffloadingSYCL)

GPUCallableMethod void tarch::accelerator::gpuAbort() {}

void* tarch::accelerator::allocateRawData(std::size_t size, tarch::memory::MemoryLocation location, int device) {
  void* data = nullptr;

  ::sycl::queue& queue = getSYCLQueue(device);

  switch (location) {
  case tarch::memory::MemoryLocation::HeapDeviceMemory:
    data = ::sycl::malloc_device<void*>(size, queue);
    break;
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    data = ::sycl::malloc_shared<void*>(size, queue);
    break;
  case tarch::memory::MemoryLocation::Heap:
  case tarch::memory::MemoryLocation::PinnedMemory:
    data = std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
    break;
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, tarch::memory::MemoryLocation location, int device) {
  assertion(data);

  ::sycl::queue& queue = getSYCLQueue(device);

  switch (location) {
  case tarch::memory::MemoryLocation::HeapDeviceMemory:
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    ::sycl::free(data, queue);
    break;
  case tarch::memory::MemoryLocation::Heap:
  case tarch::memory::MemoryLocation::PinnedMemory:
    std::free(data);
    break;
  }
}

#endif
