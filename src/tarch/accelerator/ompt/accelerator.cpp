// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)

#include "tarch/Assertions.h"

#include <omp.h>

void* tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, int device) {
  assertion(device >= 0, device);
  void* data = omp_target_alloc(sizeInBytes, device);
  assertion(data);
  return data;
}

void* tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  void* data = omp_alloc(sizeInBytes, llvm_omp_target_host_mem_alloc); // TODO: Only works with LLVM
  assertion(data);
  return data;
}

void tarch::accelerator::freeDeviceMemory(void* data, int device) {
  assertion(device >= 0, device);
  omp_target_free(data, device);
}

void tarch::accelerator::freePinnedMemory(void* data, [[maybe_unused]] int device) {
  omp_free(data, llvm_omp_target_host_mem_alloc);
}

void tarch::accelerator::copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device) {
  const int hostDevice = omp_get_initial_device();
  assertion(device >= 0, device);
  omp_target_memcpy(
    target,
    source,
    sizeInBytes,
    /*dst_offset*/ 0,
    /*src_offset*/ 0,
    /*dst_device*/ device,
    /*src_device*/ hostDevice
  );
}

void tarch::accelerator::
  copyToPinnedMemory(void* target, const void* source, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  std::memcpy(target, source, sizeInBytes);
}

void tarch::accelerator::copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device) {
  const int hostDevice = omp_get_initial_device();
  assertion(device >= 0, device);
  omp_target_memcpy(
    target,
    source,
    sizeInBytes,
    /*dst_offset*/ 0,
    /*src_offset*/ 0,
    /*dst_device*/ hostDevice,
    /*src_device*/ device
  );
}

void tarch::accelerator::
  copyFromPinnedMemory(const void* source, void* target, std::size_t sizeInBytes, [[maybe_unused]] int device) {
  std::memcpy(target, source, sizeInBytes);
}

#endif // defined(PEANO_WITH_GPU_BACKEND_OMPT)
