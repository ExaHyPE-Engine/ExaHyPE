// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/memory/memory.h"

#if defined(GPUOffloadingSYCL)

GPUCallableMethod void tarch::accelerator::gpuAbort() {}


void* tarch::accelerator::allocateRawData(std::size_t size, memory::MemoryLocation location, int device) {
  void* data = nullptr;

  switch (location) {
  case memory::MemoryLocation::Heap:
  case memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
  case memory::MemoryLocation::PinnedMemory:
    data = std::aligned_alloc(AlignmentOnHeap, tarch::padSizeToAlignment(size, AlignmentOnHeap));
    break;
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, memory::MemoryLocation location, int device) {
  assertion(data);

  switch (location) {
  case memory::MemoryLocation::Heap:
  case memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
  case memory::MemoryLocation::PinnedMemory:
    std::free(data);
    break;
  }
}


sycl::queue& tarch::accelerator::getSYCLQueue(int number) {
  assertion1(syclQueues.count(number) == 1, number);
  return syclQueues[number];
}


void tarch::accelerator::listSYCLQueues() {
  auto platforms       = sycl::platform::get_platforms();
  int  numberOfDevices = 0;
  std::cout << "SYCL system overview:" << std::endl;
  for (auto& platform : platforms) {
    auto devices = platform.get_devices();
    for (auto& device : devices) {
      std::cout
        << "device #" << numberOfDevices << ":  platform " << platform.get_info<sycl::info::platform::name>() << " / " << device.get_info<sycl::info::device::name>() << std::endl;
      numberOfDevices++;
    }
  }
}

#endif
