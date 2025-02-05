// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator");
} // namespace

#if defined(GPUOffloadingOff)

void tarch::gpuAbort() { std::abort(); }

void* tarch::accelerator::allocateRawData(
  std::size_t                     sizeInBytes,
  [[maybe_unused]] MemoryLocation location,
  int
) {
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  // Aligned alloc requires size to be a multiple of the alignment
  void* data = std::aligned_alloc(
    AlignmentOnHeap,
    padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
  );
  assertion(data);

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }

  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation, int) {
  if (!data) {
    return;
  }

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logWarning("freeRawData(...)", "attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }

  std::free(data);
}

#endif // defined(GPUOffloadingOff)

void* tarch::accelerator::reallocateRawData(
  void*          data,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(data);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  std::size_t currentSizeInBytes = 0;
  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    currentSizeInBytes = Device::getInstance().getMemoryBlockTable().at(data);
  }
  if (currentSizeInBytes >= sizeInBytes) {
    return data;
  }

  std::size_t newSizeInBytes = sizeInBytes + sizeInBytes / 2;
  void* newData = allocateRawData(newSizeInBytes, location, device);
  assertion(newData);

  // TODO: Copy host to host
  freeRawData(data, location, device);

  return newData;
}

void tarch::freeMemory(void* memoryPtr, MemoryLocation location, int device) {
  accelerator::freeRawData(memoryPtr, location, device);
}

std::size_t tarch::padSizeToAlignment(std::size_t size, std::size_t alignment) {
  assertion(size > 0);
  assertion(alignment > 0);
  // Aligned alloc requires size to be a multiple of alignment, therefore we
  // need to check it. Also see:
  // https://en.cppreference.com/w/c/memory/aligned_alloc
  // Pad to nearest multiple of alignment.
  int padding = size % alignment == 0 ? 0 : alignment - (size % alignment);
  return size + padding;
}

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double* tarch::memset(double* dest, double ch, std::size_t byteCount) {
#if defined(GPUOffloadingOff)
  assertion2(byteCount % sizeof(double) == 0, ch, byteCount);
#endif
#if defined(OpenMPManuallyOffloadMemset)
  for (size_t i = 0; i < byteCount / sizeof(double); i++) {
    dest[i] = ch;
  }
  return dest;
#else
  return static_cast<double*>(std::memset(
    static_cast<void*>(dest),
    // static_cast<int>(ch),
    ch,
    byteCount
  ));
#endif
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
