#include "tarch/multicore/multicore.h"


#if defined(SharedTBB)

double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result;
  switch (location) {
    case MemoryLocation::Heap:
      result = new (std::align_val_t(Alignment)) double[size];
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      assertionMsg(false, "not implemented yet" );
      result = new (std::align_val_t(Alignment)) double[size];
      break;
  }
  return result;
}


void tarch::freeMemory(double* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] data;
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      delete[] data;
      break;
  }
}


#endif
