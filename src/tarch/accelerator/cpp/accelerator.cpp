#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"

#if defined(GPUOffloadingCPP)

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = std::aligned_alloc(AlignmentOnHeap, tarch::padSizeToAlignment(size, AlignmentOnHeap)); // Aligned alloc requires size to be a multiple of the aligment
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__)
      CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
      #else
      result = std::aligned_alloc(AlignmentOnHeap, tarch::padSizeToAlignment(size, AlignmentOnHeap));
      #endif
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      std::free(data);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__)
      CHECK_CUDA_ERROR(cudaFree(data));
      #else
      std::free(data);
      #endif
      break;
  }
}

#endif
