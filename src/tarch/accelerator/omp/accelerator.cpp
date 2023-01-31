#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"

#include <sstream>

#if defined(GPUOffloadingOMP)

#if defined(__NVCOMPILER_CUDA__)
#include <cuda_runtime.h>

#define CHECK_CUDA_ERROR(ans) { cudaAssertion((ans), __FILE__, __LINE__); }
void cudaAssertion(cudaError_t code, const char *file, int line) {
  assertion(code == cudaSuccess);
  if (code != cudaSuccess) {
    std::stringstream ss;
    ss << "CUDA assertion: " << cudaGetErrorString(code) << " " << file << " " << line;
    throw std::runtime_error(ss.str());
  }
}
#endif

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
      #if defined(__NVCOMPILER_CUDA__)
        CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
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
      delete[] data; // TODO: This leaks memory, we should not delete a void* pointer as destructors are not called -> should refactor code into malloc/free instead.
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      #if defined(__NVCOMPILER_CUDA__)
        CHECK_CUDA_ERROR(cudaFree(data));
      #elif defined(UseAMD)
        hipFree(data);
      #else
        delete[] data;
      #endif
      break;
  }
}

#endif
