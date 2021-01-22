#include "tarch/multicore/multicore.h"


#if defined(SharedOMP)

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result;
  switch (location) {
    case MemoryLocation::Heap:
      //#pragma omp allocate(result) allocator(allocator) memspace(memspace) memtraits(optimized=latency)
      //#pragma omp allocate(result) memtraits(optimized=latency)
      result = new double[size];
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      #if defined(OpenMPGPUOffloading) and defined(UseNVIDIA)
      cudaMallocManaged((void**)&result, size*sizeof(double), cudaMemAttachGlobal);
      #elif defined(OpenMPGPUOffloading) and defined(UseAMD)
      result = new double[size];
      // AMD does not (yet) support managed memory
      // hipMallocManaged(&result, size*sizeof(double));
      #else
      // #pragma omp allocate ()
      result = new double[size];
      #endif
      break;
  }
  return result;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void tarch::freeMemory(double* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] data;
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      #if defined(OpenMPGPUOffloading) and defined(UseNVIDIA)
      cudaFree(data);
      #elif defined(OpenMPGPUOffloading) and defined(UseAMD)
      // See remark on managed memory above
      // hipFree(data);
      delete[] data;
      #else
      delete[] data;
      #endif
      break;
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


#endif

