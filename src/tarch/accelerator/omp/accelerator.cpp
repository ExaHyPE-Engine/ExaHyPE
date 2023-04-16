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
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__) // __NVCC__
      CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
      #elif defined(__HIPCC__)
      hipMallocManaged(&result, size, hipMemAttachGlobal);
      #elif defined(__AMDGPU__) or defined(__clang__) // fall back to non-managed memory when on AMD/LLVM
      result = aligned_alloc(AlignmentOnHeap, size);
      #else
      result = aligned_alloc(AlignmentOnHeap, size);
      #endif
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      free(data);
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__)
      CHECK_CUDA_ERROR(cudaFree(data));
      #elif defined(__HIPCC__)
      hipFree(data);
      #elif defined(__AMDGPU__) or defined(__clang__) // fall back to non-managed memory when on AMD/LLVM
      free(data);
      #else
      free(data);
      #endif
      break;
  }
}

#endif
