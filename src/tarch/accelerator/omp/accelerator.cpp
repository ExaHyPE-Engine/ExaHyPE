#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"

#if defined(__HIPCC__)
#include <hip/hip_runtime.h>
#endif

#if defined(GPUOffloadingOMP)

#pragma omp declare target
void tarch::gpuAbort() {}
#pragma omp end declare target

void* tarch::internal::allocateRawData(int size, MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = std::aligned_alloc(AlignmentOnHeap, size); // Aligned alloc requires size to be a multiple of the aligment
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
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
      CHECK_CUDA_ERROR(cudaFree(data));
      break;
  }
}

#endif
