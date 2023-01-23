#include "tarch/accelerator/accelerator.h"


#if defined(GPUOffloadingCPP)


double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result;
  switch (location) {
    case MemoryLocation::Heap:
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertionMsg(false, "not implemented yet" );
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
      break;
  }
  return result;
}


void tarch::freeMemory(double* data, MemoryLocation location) {
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
