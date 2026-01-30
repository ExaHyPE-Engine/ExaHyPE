// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(WITH_GPU_OMPT)

#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"

#include <cstring>
#include <omp.h>

namespace {
  tarch::logging::Log _log("tarch::accelerator::ompt");
} // namespace

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  // const int hostDevice = omp_get_initial_device();

  switch (location) {
  case MemoryLocation::Heap:
    data = allocateRawDataOnHeap(sizeInBytes);
    break;
  case MemoryLocation::Device:
    assertion1(device >= 0, device);
#ifdef WITH_USM
    data = omp_target_alloc_shared(sizeInBytes, device);
#else
    data = omp_target_alloc(sizeInBytes, device);
#endif
    break;
  case MemoryLocation::Pinned:
#ifdef WITH_USM
    assertion1(device >= 0, device);
    data = omp_target_alloc_shared(sizeInBytes, device);
#else
    data = omp_alloc(sizeInBytes, llvm_omp_target_host_mem_alloc); // TODO: Only works with LLVM
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
    omp_target_free(data, device);
    break;
  case MemoryLocation::Pinned:
#ifdef UseUSM
    assertion1(device >= 0, device);
    omp_target_free(data, device);
#else
    omp_free(data, llvm_omp_target_host_mem_alloc);
#endif
    break;
  }
}

void tarch::accelerator::
  copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  const int hostDevice = omp_get_initial_device();

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
    omp_target_memcpy(
      target,
      source,
      sizeInBytes,
      /*dst_offset*/ 0,
      /*src_offset*/ 0,
      /*dst_device*/ device,
      /*src_device*/ hostDevice
    );
    break;
  }
}

void tarch::accelerator::
  copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  const int hostDevice = omp_get_initial_device();

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
    omp_target_memcpy(
      target,
      source,
      sizeInBytes,
      /*dst_offset*/ 0,
      /*src_offset*/ 0,
      /*dst_device*/ hostDevice,
      /*src_device*/ device
    );
    break;
  }
}

#endif // defined(WITH_GPU_OMPT)
