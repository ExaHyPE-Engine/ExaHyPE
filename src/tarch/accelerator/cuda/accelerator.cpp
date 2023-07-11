#include "tarch/accelerator/accelerator.h"
#include "tarch/Assertions.h"

#if defined(GPUOffloadingCUDA)

#include <cuda_runtime.h>

#define CHECK_CUDA_ERROR(ans) { cudaAssertion((ans), __FILE__, __LINE__); }
void cudaAssertion(cudaError_t code, const char *file, int line) {
  if (code != cudaSuccess) {
    fprintf(stderr,"CUDA assertion: %s %s %d\n", cudaGetErrorString(code), file, line);
  }
  assertion(code == cudaSuccess);
}

void tarch::gpuAbort() {
}

void* tarch::internal::allocateRawData(int size, tarch::MemoryLocation location) {
  void* result = nullptr;
  switch (location) {
    case tarch::MemoryLocation::Heap:
      result = aligned_alloc(AlignmentOnHeap, size);
      break;
    case tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size));
      break;
  }
  return result;
}

void tarch::freeMemory(void* data, tarch::MemoryLocation location) {
  switch (location) {
    case tarch::MemoryLocation::Heap:
      free(data);
      break;
    case tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      CHECK_CUDA_ERROR(cudaFree(data));
      break;
  }
}

#endif
