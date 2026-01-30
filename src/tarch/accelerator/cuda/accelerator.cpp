// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(WITH_GPU_CUDA)

#include "Device.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator::cuda");
} // namespace

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    data = allocateRawDataOnHeap(sizeInBytes);
    break;
  case MemoryLocation::Device:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
#ifdef WITH_USM
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, sizeInBytes));
#else
    CHECK_CUDA_ERROR(cudaMalloc(&data, sizeInBytes));
#endif
    break;
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
#ifdef WITH_USM
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, sizeInBytes));
#else
    CHECK_CUDA_ERROR(cudaMallocHost(&data, sizeInBytes));
#endif
    break;
  }

  assertion(data);
  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);
  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation location, int device) {
  if (not data) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  switch (location) {
  case MemoryLocation::Heap:
    freeRawDataOnHeap(data);
    break;
  case MemoryLocation::Device:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
    break;
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFreeHost(data));
    break;
  }
}

void tarch::accelerator::
  copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
    CHECK_CUDA_ERROR(cudaSetDevice(device));
#ifdef WITH_USM
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
#else
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToDevice, deviceStreamGet()));
#endif
    break;
  }
}

void tarch::accelerator::
  copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from host to host");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
    CHECK_CUDA_ERROR(cudaSetDevice(device));
#ifdef WITH_USM
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
#else
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyDeviceToHost, deviceStreamGet()));
#endif
    break;
  }
}

#endif // defined(WITH_GPU_CUDA)
