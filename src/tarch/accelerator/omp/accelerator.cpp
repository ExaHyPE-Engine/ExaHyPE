#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"

#if defined(GPUOffloadingOMP)

#pragma omp declare target
void tarch::gpuAbort() {}
#pragma omp end declare target

void* tarch::internal::allocateRawData(
  std::size_t     size,
  MemoryLocation  location,
  int             /*device*/
) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    data = std::aligned_alloc(
      AlignmentOnHeap,
      padSizeToAlignment(size, AlignmentOnHeap)
    );
    break;
  }

  assertion(data);
  return data;
}

void tarch::freeMemory(
  void*           data,
  MemoryLocation  location,
  int             /*device*/
) {
  switch (location) {
  case MemoryLocation::Heap:
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    if (data != nullptr) {
      std::free(data);
      data = nullptr;
    }
    break;
  }
}

#endif // defined(GPUOffloadingOMP)
