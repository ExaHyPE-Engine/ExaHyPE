// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/hip/Device.h"

namespace {
  void activateDevice(int device) {
    int currentDevice = -1;
    CHECK_HIP_ERROR(hipGetDevice(&currentDevice));
    if (currentDevice != device) {
      CHECK_HIP_ERROR(hipSetDevice(device));
    }
  }
} // namespace

void*
tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMalloc(&data, sizeInBytes));
  assertion(data);
  return data;
}

void*
tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  assertion(device >= 0, device);
  CHECK_HIP_ERROR(hipHostMalloc(&data, sizeInBytes));
  assertion(data);
  return data;
}

void
tarch::accelerator::freeDeviceMemory(void* data, int device) {
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipFree(data));
}

void
tarch::accelerator::freePinnedMemory(void* data, int device) {
  assertion(device >= 0, device);
  CHECK_HIP_ERROR(hipHostFree(data));
}

void
tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToDevice, getDeviceStream()));
  // A copy from pinned host memory returns before the transfer ends, so the caller could overwrite its source.
  CHECK_HIP_ERROR(hipStreamSynchronize(getDeviceStream()));
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
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyDeviceToHost, getDeviceStream()));
  // A copy into pinned host memory returns before the transfer ends, so the caller could read stale values.
  CHECK_HIP_ERROR(hipStreamSynchronize(getDeviceStream()));
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
 * Blocks until every launch and copy on the stream of the current device has completed. A
 * parallelFor() on this backend returns before its launch completes; copyFrom() and every
 * reduction wait for the stream themselves, so a caller waits here only before it reads device
 * memory in another way, such as through MPI.
 */
void
tarch::accelerator::waitForDevice() {
  CHECK_HIP_ERROR(hipStreamSynchronize(getDeviceStream()));
}

#endif // defined(PEANO_WITH_GPU_BACKEND_HIP)
