// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "MemoryLocation.h"

#include <cstddef>

namespace tarch::accelerator {
  struct MemoryAllocationCache {
    static MemoryAllocationCache& getInstance();

    void* allocate(std::size_t sizeInBytes, MemoryLocation location, int device);
    void  free(void* pointer, MemoryLocation location, int device);
    void  clear();

    MemoryAllocationCache(const MemoryAllocationCache&)            = delete;
    MemoryAllocationCache& operator=(const MemoryAllocationCache&) = delete;
    MemoryAllocationCache(MemoryAllocationCache&&)                 = delete;
    MemoryAllocationCache& operator=(MemoryAllocationCache&&)      = delete;

  private:
    MemoryAllocationCache()  = default;
    ~MemoryAllocationCache() = default;
  };
} // namespace tarch::accelerator
