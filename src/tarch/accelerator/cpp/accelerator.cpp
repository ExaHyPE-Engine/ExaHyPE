#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingCPP)

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertionMsg(false, "not implemented yet" );
      result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] data;
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      delete[] data;
      break;
  }
}

#endif
