#include "tarch/multicore/multicore.h"


#if defined(SharedCPP)

double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result;
  switch (location) {
    case MemoryLocation::Heap:
      //#pragma omp allocate(result) allocator(allocator) memspace(memspace) memtraits(optimized=latency)
      //#pragma omp allocate(result) memtraits(optimized=latency)
      result = new double[size];
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      result = new double[size];
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
