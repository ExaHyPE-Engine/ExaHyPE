#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"

#if defined(GPUOffloadingSYCL)
GPUCallableMethod void tarch::gpuAbort() {}

void* tarch::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      //result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertionMsg(false, "not implemented yet" );
      //result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      //delete[] data;
      free(data);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      //delete[] data;
      free(data);
      break;
  }
}

#endif
