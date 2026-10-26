// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/Assertions.h"
#include "tarch/accelerator/cuda/Device.h"

namespace {
  void activateDevice(int device) {
    int currentDevice = -1;
    CHECK_CUDA_ERROR(cudaGetDevice(&currentDevice));
    if (currentDevice != device) {
      CHECK_CUDA_ERROR(cudaSetDevice(device));
    }
  }
} // namespace

void*
tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  ASSERTION(device >= 0, device);
  activateDevice(device);
  CHECK_CUDA_ERROR(cudaMalloc(&data, sizeInBytes));
  ASSERTION(data);
  return data;
}

void*
tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  ASSERTION(device >= 0, device);
  CHECK_CUDA_ERROR(cudaMallocHost(&data, sizeInBytes));
  ASSERTION(data);
  return data;
}

void
tarch::accelerator::freeDeviceMemory(void* data, int device) {
  ASSERTION(device >= 0, device);
  activateDevice(device);
  CHECK_CUDA_ERROR(cudaFree(data));
}

void
tarch::accelerator::freePinnedMemory(void* data, int device) {
  ASSERTION(device >= 0, device);
  CHECK_CUDA_ERROR(cudaFreeHost(data));
}

void
tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  ASSERTION(device >= 0, device);
  activateDevice(device);
  CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyHostToDevice, getDeviceStream()));
  // A copy from pinned host memory returns before the transfer ends. The caller could then overwrite its source.
  CHECK_CUDA_ERROR(cudaStreamSynchronize(getDeviceStream()));
}

void
tarch::accelerator::copyToPinnedMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

void
tarch::accelerator::copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device) {
  ASSERTION(device >= 0, device);
  activateDevice(device);
  CHECK_CUDA_ERROR(cudaMemcpyAsync(target, source, sizeInBytes, cudaMemcpyDeviceToHost, getDeviceStream()));
  // A copy into pinned host memory returns before the transfer ends. The caller could then read stale values.
  CHECK_CUDA_ERROR(cudaStreamSynchronize(getDeviceStream()));
}

void
tarch::accelerator::copyFromPinnedMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

/**
 * Blocks until every launch and copy on the stream of the current device has completed. copyFrom()
 * and every reduction wait for the stream themselves. A caller waits here before it reads device
 * memory in another way, such as through MPI.
 */
void
tarch::accelerator::waitForDevice() {
  CHECK_CUDA_ERROR(cudaStreamSynchronize(getDeviceStream()));
}

#endif // defined(PEANO_WITH_GPU_BACKEND_CUDA)
