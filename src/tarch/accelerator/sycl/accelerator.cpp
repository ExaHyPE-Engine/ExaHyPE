#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"

#if defined(GPUOffloadingSYCL)
GPUCallableMethod void tarch::gpuAbort() {}

void* tarch::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertionMsg(false, "not implemented yet" );
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      free(data);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      free(data);
      break;
  }
}

#endif
