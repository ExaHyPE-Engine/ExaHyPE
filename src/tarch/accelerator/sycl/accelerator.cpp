// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"

#if defined(GPUOffloadingSYCL)

#include "Device.h"

GPUCallableMethod void tarch::gpuAbort() {}

void* tarch::internal::allocateRawData(
  std::size_t           size,
  MemoryLocation        location,
  int                   device
) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == accelerator::Device::HostDevice, device);
    data = std::aligned_alloc(
      AlignmentOnHeap,
      tarch::padSizeToAlignment(size, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    assertion1(device >= 0, device);
    ::sycl::queue& queue = accelerator::getSYCLQueue(device);
    data = ::sycl::malloc_shared<void*>(size, queue);
    break;
  }

  assertion(data);
  return data;
}

void tarch::freeMemory(void* data, MemoryLocation location, int device) {
  switch (location) {
    case MemoryLocation::Heap:
      assertion1(device == accelerator::Device::HostDevice, device);
      if (data!=nullptr) {
        std::free(data);
      }
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertion1(device >= 0, device);
      assertion(data!=nullptr);
      ::sycl::queue& queue = accelerator::getSYCLQueue(device);
      ::sycl::free(data, queue);
      break;
  }
}

#endif
