#include "accelerator.h"


#if defined(GPUOffloadingOff)
double* tarch::allocateMemory(int size, MemoryLocation location) {
 return new (std::align_val_t(AlignmentOnHeap)) double[size];
}


void tarch::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}


void tarch::gpuAbort() {
  std::abort();
}
#endif

