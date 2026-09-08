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

void* tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMalloc(&data, sizeInBytes));
  assertion(data);
  return data;
}

void* tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, int device) {
  void* data = nullptr;
  assertion(device >= 0, device);
  CHECK_HIP_ERROR(hipHostMalloc(&data, sizeInBytes));
  assertion(data);
  return data;
}

void tarch::accelerator::freeDeviceMemory(void* data, int device) {
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipFree(data));
}

void tarch::accelerator::freePinnedMemory(void* data, int device) {
  assertion(device >= 0, device);
  CHECK_HIP_ERROR(hipHostFree(data));
}

void tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToDevice, deviceStreamGet()));
}

void tarch::accelerator::copyToPinnedMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  activateDevice(device);
  CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyDeviceToHost, deviceStreamGet()));
}

void tarch::accelerator::copyFromPinnedMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

#endif // defined(PEANO_WITH_GPU_BACKEND_HIP)
