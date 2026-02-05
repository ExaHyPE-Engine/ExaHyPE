#include "accelerator.h"

#include "tarch/Assertions.h"

#include <cstring>

std::string tarch::toString(MemoryLocation value) {
  switch (value) {
  case MemoryLocation::Heap:
    return "heap";
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    return "managed-shared-accelerator-device";
  }
  return "<undef>";
}

#if defined(GPUOffloadingOff)

void* tarch::internal::allocateRawData(std::size_t size, [[maybe_unused]] MemoryLocation location, int) {
  assertion2(size > 0, size, toString(location));
  // Aligned alloc requires size to be a multiple of the alignment
  void* data = std::aligned_alloc(
    AlignmentOnHeap,
    padSizeToAlignment(size, AlignmentOnHeap)
  );
  assertion(data);
  return data;
}

void tarch::freeMemory(void* data, MemoryLocation, int) {
  if (data!=nullptr) {
    std::free(data);
  }
}

void tarch::gpuAbort() { std::abort(); }

#endif

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