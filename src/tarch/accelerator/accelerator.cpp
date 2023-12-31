// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "accelerator.h"

#include <cstring>

#include "tarch/Assertions.h"
#include "tarch/memory/memory.h"

#if defined(GPUOffloadingOff)

void tarch::accelerator::gpuAbort() { std::abort(); }

void* tarch::internal::allocateRawData(std::size_t size, memory::MemoryLocation, int) {
  assertion2(size > 0, size, toString(location));
  // Aligned alloc requires size to be a multiple of the alignment
  void* data = std::aligned_alloc(AlignmentOnHeap, padSizeToAlignment(size, AlignmentOnHeap));
  assertion(data);
  return data;
}

void tarch::accelerator::freeRawData(void* data, memory::MemoryLocation, int) {
  assertion(data);
  std::free(data);
}

void tarch::accelerator::copyTo(void* dst, const void* src, std::size_t count) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copyFrom(void* dst, const void* src, std::size_t count) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copyBetween(void* dst, const void* src, std::size_t count) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copyToAsync(void* dst, const void* src, std::size_t count, void*) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copyFromAsync(void* dst, const void* src, std::size_t count, void*) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copyBetweenAsync(void* dst, const void* src, std::size_t count, void*) {
  assertion(dst);
  assertion(src);
  std::memcpy(dst, src, count);
}

void tarch::accelerator::copy2DTo(void*, std::size_t, const void*, std::size_t, std::size_t, std::size_t) {}

void tarch::accelerator::copy2DFrom(void*, std::size_t, const void*, std::size_t, std::size_t, std::size_t) {}

void tarch::accelerator::prefetchUnifiedMemoryTo(const void*, std::size_t, void*) {}

#endif

int tarch::padSizeToAlignment(std::size_t size, int alignment) {
  // Aligned alloc requires size to be a multiple of alignment, therefore we need to check it.
  // Also see: https://en.cppreference.com/w/c/memory/aligned_alloc
  // Pad to nearest multiple of alignment.
  int padding = size % alignment == 0 ? 0 : alignment - (size % alignment);
  return size + padding;
}
