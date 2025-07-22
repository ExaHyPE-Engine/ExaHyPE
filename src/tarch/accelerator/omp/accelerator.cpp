// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingOMP)

#ifndef _OPENMP
#error "_OPENMP is not defined. Please compile with OpenMP support enabled."
#endif

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <omp.h>
#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator::omp");
} // namespace

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
      ALIGNMENT_ON_HEAP,
      tarch::padSizeToAlignment(sizeInBytes, ALIGNMENT_ON_HEAP)
    );
    break;
  case MemoryLocation::Device:
#ifdef UseUSM
    data = omp_alloc(sizeInBytes, llvm_omp_target_shared_mem_alloc);
#else
    assertion1(device >= 0, device);
    data = omp_target_alloc(sizeInBytes, device);
#endif
    break;
  case MemoryLocation::Pinned:
#ifdef UseUSM
    data = omp_alloc(sizeInBytes, llvm_omp_target_shared_mem_alloc);
#else
    data = omp_alloc(sizeInBytes, llvm_omp_target_host_mem_alloc);
#endif
    break;
  }

  assertion(data);

#if PeanoDebug > 0
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }
#endif

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

#if PeanoDebug > 0
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logDebug("freeRawData()", "Attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }
#endif

  switch (location) {
  case MemoryLocation::Heap:
    std::free(data); //omp_free(data);
    break;
  case MemoryLocation::Device:
#ifdef UseUSM
    omp_free(data, llvm_omp_target_shared_mem_alloc);
#else
    assertion1(device >= 0, device);
    omp_target_free(data, device);
#endif
    break;
  case MemoryLocation::Pinned:
#ifdef UseUSM
    omp_free(data, llvm_omp_target_shared_mem_alloc);
#else
    omp_free(data, llvm_omp_target_host_mem_alloc);
#endif
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
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
#ifdef UseUSM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    omp_target_memcpy(target, source, sizeInBytes, 0, 0, device, omp_get_initial_device());
#endif
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
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
#ifdef UseUSM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    omp_target_memcpy(target, source, sizeInBytes, 0, 0, omp_get_initial_device(), device);
#endif
    break;
  }
}

#endif // defined(GPUOffloadingOMP)
