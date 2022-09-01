#include "tarch/multicore/multicore.h"


#if defined(SharedOMP)

/*
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
*/
double* tarch::allocateMemory(int size, MemoryLocation location) {
  double* result = nullptr;
  switch (location) {
    case MemoryLocation::Heap:
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      //#if defined(GPUOffloadingOMP) and defined(UseNVIDIA)
      //cudaMallocManaged((void**)&result, size*sizeof(double), cudaMemAttachGlobal);
      //#else
      //// #pragma omp allocate ()
      result = new (std::align_val_t(AlignmentOnHeap)) double[size];
      //#endif
      break;
  }
  return result;
}
/*
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
*/


/*
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
*/
void tarch::freeMemory(double* data, MemoryLocation location) {
  switch (location) {
    case MemoryLocation::Heap:
      delete[] data;
      break;
    case MemoryLocation::ManagedAcceleratorMemory:
      //#if defined(GPUOffloadingOMP) and defined(UseNVIDIA)
      //cudaFree(data);
      //#elif defined(GPUOffloadingOMP) and defined(UseAMD)
      //// See remark on managed memory above
      //// hipFree(data);
      //delete[] data;
      //#else
      delete[] data;
      //#endif
      //break;
  }
}
/*
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
*/


#endif

