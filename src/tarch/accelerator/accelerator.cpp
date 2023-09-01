#include "accelerator.h"

#if defined(GPUOffloadingOff)

void* tarch::internal::allocateRawData(int size, [[maybe_unused]] MemoryLocation location) {
  return aligned_alloc(AlignmentOnHeap, size);
}

void tarch::freeMemory(void* data, [[maybe_unused]] MemoryLocation location) {
  free(data);
}

void tarch::gpuAbort() {
  std::abort();
}

#endif
