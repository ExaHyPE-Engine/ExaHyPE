#include "tarch/accelerator/accelerator.h"

#include "tarch/Assertions.h"

#if defined(GPUOffloadingHIP) && defined(__HIPCC__)

/**
 * Will be needed to couple HIP with OpenMP for AMD GPUs.
 */
#include <hip/hip_runtime.h>

void tarch::gpuAbort() {}

void* tarch::internal::allocateRawData(
  std::size_t           size,
  MemoryLocation        location,
  int                   device
) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  // TODO: Use HIP routines
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

void tarch::freeMemory(void* data, MemoryLocation location, int device) {
  // TODO: Use HIP routines
  switch (location) {
  case MemoryLocation::Heap:
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    if (data!=nullptr) {
      std::free(data);
    }
    break;
  }
}

#endif
