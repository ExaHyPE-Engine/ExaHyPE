// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/memory/memory.h"

#if defined(GPUOffloadingOMP)

#pragma omp declare     target
void                    tarch::accelerator::gpuAbort() {}
#pragma omp end declare target


void* tarch::accelerator::allocateRawData(std::size_t size, tarch::memory::MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
  case tarch::memory::MemoryLocation::Heap:
    data = omp_target_alloc(size, device);
    // data = std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
    break;
  case tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
#if defined(__NVCOMPILER_CUDA__) // __NVCC__
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, size));
#else
    data = omp_target_alloc(size, device);
#endif
    break;
  case tarch::memory::MemoryLocation::PinnedMemory:
#if defined(__NVCOMPILER_CUDA__) // __NVCC__
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocHost(&data, size));
#else
    data = omp_target_alloc(size, device);
#endif
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, tarch::memory::MemoryLocation location, int device) {
  assertion(data);

  switch (location) {
  case tarch::memory::MemoryLocation::Heap:
    // omp_target_free(data, device);
    std::free(data);
    break;
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
#if defined(__NVCOMPILER_CUDA__)
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
#else
    omp_target_free(data, device);
#endif
    break;
  case tarch::memory::MemoryLocation::PinnedMemory:
#if defined(__NVCOMPILER_CUDA__)
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFreeHost(data));
#else
    omp_target_free(data, device);
#endif
    break;
  }
}

#endif
