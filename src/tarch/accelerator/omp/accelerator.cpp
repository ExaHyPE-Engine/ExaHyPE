#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"

#if defined(GPUOffloadingOMP)

#pragma omp declare target
void tarch::gpuAbort() {}
#pragma omp end declare target

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
    // data = omp_target_alloc(size, device);
    data = std::aligned_alloc(
      AlignmentOnHeap,
      padSizeToAlignment(size, AlignmentOnHeap)
    );
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
    // omp_target_free(data, device);
    std::free(data);
    break;
  }
}

#endif
