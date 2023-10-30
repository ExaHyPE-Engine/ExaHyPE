#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"

#if defined(GPUOffloadingSYCL)
GPUCallableMethod void tarch::gpuAbort() {}

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = std::aligned_alloc(AlignmentOnHeap, size);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertionMsg(false, "not implemented yet" );
      result = std::aligned_alloc(AlignmentOnHeap, size);
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      std::free(data);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      std::free(data);
      break;
  }
}

#endif
