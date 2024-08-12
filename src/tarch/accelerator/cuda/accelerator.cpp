#include "tarch/accelerator/accelerator.h"

#include "ErrorCheck.h"
#include "tarch/Assertions.h"

#if defined(GPUOffloadingCUDA)

#include <cuda_runtime.h>

void tarch::gpuAbort() {}

void* tarch::internal::allocateRawData(
  std::size_t           size,
  MemoryLocation        location,
  int                   device
) {
  void* data = nullptr;
  assertion2(size > 0, size, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == accelerator::Device::HostDevice, device);
    data = std::aligned_alloc(
      AlignmentOnHeap,
      padSizeToAlignment(size, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    assertion1(device >= 0, device);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaMallocManaged(&data, size));
    break;
  }

  assertion(data);
  return data;
}

void tarch::freeMemory(void* data, MemoryLocation location, int device) {
  switch (location) {
    case MemoryLocation::Heap:
      assertion1(device == accelerator::Device::HostDevice, device);
      if (data!=nullptr) {
        std::free(data);
      }
      break;
    case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
      assertion1(device >= 0, device);
      assertion(data!=nullptr);
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(data));
    break;
  }
}

#endif
