#include "tarch/multicore/multicore.h"
#include "tarch/Assertions.h"

#if defined(SharedOMP)

#if defined(GPUOffloadingOMP) and defined(__NVCOMPILER_CUDA__)
#include <cuda_runtime.h>
#define CHECK_CUDA_ERROR(ans) { cudaAssertion((ans), __FILE__, __LINE__); }
void cudaAssertion(cudaError_t code, const char *file, int line) {
   if (code != cudaSuccess) {
      fprintf(stderr,"CUDA assertion: %s %s %d\n", cudaGetErrorString(code), file, line);
   }
   assertion(code == cudaSuccess);
}
#endif

double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
#if defined(GPUOffloadingOMP) and defined(__NVCOMPILER_CUDA__)
      CHECK_CUDA_ERROR(cudaMallocManaged((void**)&result, size*sizeof(double)));
#else
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
#endif
      break;
  }
  return result;
}

void tarch::freeMemory(double* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] data;
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
#if defined(GPUOffloadingOMP) and defined(__NVCOMPILER_CUDA__)
      CHECK_CUDA_ERROR(cudaFree(data));
#elif defined(GPUOffloadingOMP) and defined(UseAMD)
      hipFree(data);
#else
      delete[] data;
#endif
      break;
  }
}

#endif
