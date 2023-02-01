#include "accelerator.h"

#if defined(GPUOffloadingOff)

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  return new (std::align_val_t(AlignmentOnHeap)) char[size];
}

void tarch::freeMemory(void* data, [[maybe_unused]] MemoryLocation location) {
  delete[] data;
}

void tarch::gpuAbort() {
  std::abort();
}

#endif
