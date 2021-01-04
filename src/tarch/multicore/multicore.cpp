#include "multicore.h"

#ifndef SharedMemoryParallelisation

double* tarch::multicore::allocateMemory(int size, MemoryLocation location) {
 return new double[size];
}


void tarch::multicore::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}
#endif
