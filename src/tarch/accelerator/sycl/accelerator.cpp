// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include "Device.h"
#include "tarch/Assertions.h"

#include <cstring>
#include <sycl/sycl.hpp>

void* tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  void*        data  = sycl::malloc_device(sizeInBytes, queue);
  assertion(data);
  return data;
}

void* tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  void*        data  = sycl::malloc_host(sizeInBytes, queue);
  assertion(data);
  return data;
}

void tarch::accelerator::freeDeviceMemory(void* data, int device) {
  assertion(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  sycl::free(data, queue);
}

void tarch::accelerator::freePinnedMemory(void* data, int device) { freeDeviceMemory(data, device); }

void tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  queue.memcpy(target, source, sizeInBytes).wait();
}

void tarch::accelerator::
  copyToPinnedMemory(void* target, const void* source, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
  queue.memcpy(target, source, sizeInBytes).wait();
}

void tarch::accelerator::
  copyFromPinnedMemory(const void* source, void* target, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  std::memcpy(target, source, sizeInBytes);
}

#endif // defined(PEANO_WITH_GPU_BACKEND_SYCL)
