// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingHIP) && defined(__HIPCC__)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <hip/hip_runtime.h>

namespace {
  tarch::logging::Log _log("tarch::accelerator::hip");
} // namespace

void* tarch::internal::allocateRawData(
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
  case MemoryLocation::Device:
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
    std::free(data);
    break;
  }
}

#endif // defined(GPUOffloadingHIP) && defined(__HIPCC__)
