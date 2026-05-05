// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(WITH_GPU_HIP)

#include "Device.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"

#include <cstring>

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    data = allocateRawDataOnHeap(sizeInBytes);
    break;
  case MemoryLocation::Device:
    assertion1(device >= 0, device);
    CHECK_HIP_ERROR(hipSetDevice(device));
#ifdef WITH_USM
    CHECK_HIP_ERROR(hipMallocManaged(&data, sizeInBytes));
#else
    CHECK_HIP_ERROR(hipMalloc(&data, sizeInBytes));
#endif
    break;
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    CHECK_HIP_ERROR(hipSetDevice(device));
#ifdef WITH_USM
    CHECK_HIP_ERROR(hipMallocManaged(&data, sizeInBytes));
#else
    CHECK_HIP_ERROR(hipHostMalloc(&data, sizeInBytes));
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
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipFree(data));
    break;
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipHostFree(data));
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
    logDebug("Copy " << sizeInBytes << " bytes from host to host");
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("Copy " << sizeInBytes << " bytes from host to device");
    CHECK_HIP_ERROR(hipSetDevice(device));
#ifdef WITH_USM
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToHost, deviceStreamGet()));
#else
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToDevice, deviceStreamGet()));
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
    logDebug("Copy " << sizeInBytes << " bytes from host to host");
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToHost, deviceStreamGet()));
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    logDebug("Copy " << sizeInBytes << " bytes from device to host");
    CHECK_HIP_ERROR(hipSetDevice(device));
#ifdef WITH_USM
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyHostToHost, deviceStreamGet()));
#else
    CHECK_HIP_ERROR(hipMemcpyAsync(target, source, sizeInBytes, hipMemcpyDeviceToHost, deviceStreamGet()));
#endif
    break;
  }
}

#endif // defined(WITH_GPU_HIP)
