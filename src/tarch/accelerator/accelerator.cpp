#include "accelerator.h"

#if defined(GPUOffloadingOff)

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  //return new (std::align_val_t(AlignmentOnHeap)) char[size];
  return aligned_alloc(AlignmentOnHeap, size);
}

void tarch::freeMemory(void* data, [[maybe_unused]] MemoryLocation location) {
  //delete[] data;
  free(data);
}

void tarch::gpuAbort() {
  std::abort();
}

#endif
