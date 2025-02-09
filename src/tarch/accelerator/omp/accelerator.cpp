// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingOMP)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <omp.h>
#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator::omp");
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
    data = std::aligned_alloc( //omp_aligned_alloc(
      AlignmentOnHeap,
      tarch::padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    data = omp_target_alloc(sizeInBytes, device);
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    // OpenMP does not support explicit USM allocations, therefore we allocate heap memory on the host.
    data = std::aligned_alloc( //omp_aligned_alloc(
      AlignmentOnHeap,
      padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
    );
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
      logDebug("freeRawData(...)", "attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }

  switch (location) {
  case MemoryLocation::Heap:
    std::free(data); //omp_free(data);
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    omp_target_free(data, device);
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    std::free(data); //omp_free(data);
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
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to device");
    omp_target_memcpy(target, source, sizeInBytes, 0, 0, device, omp_get_initial_device());
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
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
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::DeviceHeap:
    assertion1(device >= 0, device);
    logDebug("copyFrom(...)", "copy " << sizeInBytes << " bytes from device to host");
    omp_target_memcpy(target, source, sizeInBytes, 0, 0, omp_get_initial_device(), device);
    break;
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    logDebug("copyTo(...)", "copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  }
}

#endif // defined(GPUOffloadingOMP)
