#include "accelerator.h"

#if defined(GPUOffloadingOff)

template <class T>
double* tarch::allocateMemory(int size, MemoryLocation location) {
  return new (std::align_val_t(AlignmentOnHeap)) double[size];
}

void tarch::freeMemory(double* data, [[maybe_unused]] MemoryLocation location) {
  delete[] data;
}

void tarch::gpuAbort() {
  std::abort();
}

#endif
