#include "accelerator.h"
#include "tarch/Assertions.h"


std::string tarch::toString( MemoryLocation value ) {
  switch (value) {
    case MemoryLocation::Heap:
      return "heap";
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      return "managed-shared-accelerator-device";
  }
  return "<undef>";
}


#if defined(GPUOffloadingOff)

void* tarch::internal::allocateRawData(int size, [[maybe_unused]] MemoryLocation location) {
  assertion2( size>0, size, toString(location) );
  return std::aligned_alloc(AlignmentOnHeap, size);
}

void tarch::freeMemory(void* data, [[maybe_unused]] MemoryLocation location) {
  std::free(data);
}

void tarch::gpuAbort() {
  std::abort();
}

#endif
