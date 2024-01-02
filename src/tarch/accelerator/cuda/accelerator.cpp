// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#include "ErrorCheck.h"
#include "tarch/Assertions.h"
#include "tarch/memory/memory.h"

#if defined(GPUOffloadingCUDA)

#include <cuda_runtime.h>

void tarch::accelerator::gpuAbort() {}

void* tarch::accelerator::allocateRawData(std::size_t size, tarch::memory::MemoryLocation location, int device) {
  void* data = nullptr;

  switch (location) {
  case tarch::memory::MemoryLocation::Heap:
    data = std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
    break;
  case tarch::memory::MemoryLocation::HeapDeviceMemory:
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMalloc(&data, size));
    break;
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, size));
    break;
  case tarch::memory::MemoryLocation::PinnedMemory:
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocHost(&data, size));
    break;
  }

  assertion(data);
  return data;
}


void tarch::accelerator::freeRawData(void* data, tarch::memory::MemoryLocation location, int device) {
  assertion(data);

  switch (location) {
  case tarch::memory::MemoryLocation::Heap:
    std::free(data);
    break;
  case tarch::memory::MemoryLocation::HeapDeviceMemory:
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
    break;
  case tarch::memory::MemoryLocation::PinnedMemory:
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFreeHost(data));
    break;
  }
}

#endif
