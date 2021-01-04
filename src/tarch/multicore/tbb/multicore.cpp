#include "tarch/multicore/multicore.h"

#if defined(SharedTBB)
double* tarch::allocateMemory(int size, MemoryLocation location) {
  return new double[size];
}


void tarch::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}
#endif
