// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingSYCL)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

void tarch::gpuAbort() {}

void* tarch::accelerator::allocateRawData(
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == accelerator::Device::HostDevice, device);
    data = std::aligned_alloc( // ::sycl::malloc_host
      AlignmentOnHeap,
      tarch::padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    ::sycl::queue& queue = accelerator::getSYCLQueue(device);
    data                 = ::sycl::malloc_device<void*>(sizeInBytes, queue);
    break;
  }
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    assertion1(device >= 0, device);
    ::sycl::queue& queue = accelerator::getSYCLQueue(device);
    data                 = ::sycl::malloc_shared<void*>(sizeInBytes, queue);
    break;
  }

  assertion(data);

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }

  return data;
}

void tarch::accelerator::freeRawData(
  void*          data,
  MemoryLocation location,
  int            device
) {
  if (!data) {
    return;
  }

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logWarning("freeRawData(...)", "attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }

  switch (location) {
    case MemoryLocation::Heap:
      assertion1(device == accelerator::Device::HostDevice, device);
      std::free(data); // ::sycl::free
      break;
    case MemoryLocation::DeviceHeap:
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertion1(device >= 0, device);
      ::sycl::queue& queue = accelerator::getSYCLQueue(device);
      ::sycl::free(data, queue);
      break;
  }
}

#endif // defined(GPUOffloadingSYCL)
