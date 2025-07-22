// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingCPP)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator::cpp");
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
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    data = std::aligned_alloc(
      ALIGNMENT_ON_HEAP,
      padSizeToAlignment(sizeInBytes, ALIGNMENT_ON_HEAP)
    );
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
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    std::free(data);
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
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
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
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  }
}

#endif // defined(GPUOffloadingCPP)
