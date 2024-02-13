#include "accelerator.h"

#include "tarch/Assertions.h"


#include <cstring>

std::string tarch::toString(MemoryLocation value) {
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
  assertion2(size > 0, size, toString(location));
  // Aligned alloc requires size to be a multiple of the aligment
  return std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
}

void tarch::freeMemory(void* data, [[maybe_unused]] MemoryLocation location) { std::free(data); }

void tarch::gpuAbort() { std::abort(); }

#endif

int tarch::padSizeToAlignment(int size, int aligment){
  // Aligned alloc requires size to be a multiple of Aligment, therefore we need to check it
  // Also see: https://en.cppreference.com/w/c/memory/aligned_alloc
  // Pad to nearest multiple of aligment
  int padding = size % aligment == 0 ? 0 : aligment - (size % aligment);
  return size + padding;
}

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double* tarch::memset(double* dest, double ch, size_t byteCount){
  #if defined(OpenMPManuallyOffloadMemset)
  for (size_t i=0; i<byteCount/sizeof(double); i++){
    dest[i] = ch;
  }
  return dest;
  #else
  return static_cast<double*>(
    std::memset(
      static_cast<void*>(dest),
      //static_cast<int>(ch),
      ch,
      byteCount
    )
  );
  #endif
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
