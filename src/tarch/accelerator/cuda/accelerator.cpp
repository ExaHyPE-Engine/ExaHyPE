// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingCUDA)

#include "Device.h"
#include "ErrorCheck.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

namespace {
  tarch::logging::Log _log("tarch::accelerator::cuda");
} // namespace

void tarch::gpuAbort() {}

void* tarch::accelerator::allocateRawData(
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == accelerator::Device::HostDevice, device);
    data = std::aligned_alloc( // cudaMallocHost(flags) -> pinned memory with flags
      AlignmentOnHeap,
      padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMalloc(&data, sizeInBytes));
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, sizeInBytes));
    break;
  }

  assertion(data);

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }

  return data;
}

void tarch::accelerator::freeRawData(
  void*          data,
  MemoryLocation location,
  int            device
) {
  if (!data) {
    return;
  }

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logWarning("freeRawData(...)", "attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == accelerator::Device::HostDevice, device);
    std::free(data); // cudaFreeHost
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
    break;
  }
}

void tarch::accelerator::copyTo(
  void*          target,
  const void*    source,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to host");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to device");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToDevice, deviceStreamGet()));
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    // Do nothing
    break;
  }
}

void tarch::accelerator::copyFrom(
  const void*    source,
  void*          target,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyFrom(...)", "copy " << sizeInBytes << " bytes from host to host");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    logDebug("copyFrom(...)", "copy " << sizeInBytes << " bytes from device to host");
    CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyDeviceToHost, deviceStreamGet()));
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    // Do nothing
    break;
  }
}

#endif // defined(GPUOffloadingCUDA)
