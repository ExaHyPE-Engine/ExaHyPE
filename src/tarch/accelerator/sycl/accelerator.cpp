// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include "tarch/Assertions.h"
#include "tarch/accelerator/sycl/Device.h"

#include <sycl/sycl.hpp>

void*
tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  ASSERTION(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  void*        data  = sycl::malloc_device(sizeInBytes, queue);
  ASSERTION(data);
  return data;
}

void*
tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, int device) {
  ASSERTION(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  void*        data  = sycl::malloc_host(sizeInBytes, queue);
  ASSERTION(data);
  return data;
}

void
tarch::accelerator::freeDeviceMemory(void* data, int device) {
  ASSERTION(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  sycl::free(data, queue);
}

void
tarch::accelerator::freePinnedMemory(void* data, int device) {
  freeDeviceMemory(data, device);
}

void
tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  ASSERTION(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  queue.memcpy(target, source, sizeInBytes).wait();
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
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  queue.memcpy(target, source, sizeInBytes).wait();
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
 * Blocks until the queue of the current device has completed its work.
 */
void
tarch::accelerator::waitForDevice() {
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(getCurrentDevice()));
  queue.wait();
}

#endif // defined(PEANO_WITH_GPU_BACKEND_SYCL)
