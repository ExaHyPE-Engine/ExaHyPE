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
      result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__) // __NVCC__
        CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
      #elif defined(__HIPCC__)
        hipMallocManaged(&result, size, hipMemAttachGlobal);
      #elif defined(__AMDGPU__) or defined(__clang__) // fall back to non-managed memory when on AMD/LLVM
        result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      #else
        result = new (std::align_val_t(AlignmentOnHeap)) char[size];
      #endif
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] reinterpret_cast<char*>(data); // @TODO: This leaks memory, we should not delete a void* pointer as destructors are not called -> should refactor code into malloc/free instead.
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__)
        CHECK_CUDA_ERROR(cudaFree(data));
      #elif defined(__HIPCC__)
        hipFree(data);
      #elif defined(__AMDGPU__) or defined(__clang__) // fall back to non-managed memory when on AMD/LLVM
        delete[] data;
      #else
        delete[] data;
      #endif
      break;
  }
}

#endif
