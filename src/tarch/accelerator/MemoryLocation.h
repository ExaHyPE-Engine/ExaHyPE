// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
  enum class MemoryLocation {
    /**
     * Places the data on the heap of the host.
     */
    Heap,

    /**
     * Places the data on the device. A build without a GPU backend places it on the heap.
     */
    Device,

    /**
     * Places the data in page-locked host memory.
     */
    Pinned
  };

  struct AllocatedBytes;

  std::string toString(MemoryLocation value);

  INLINE_METHOD AllocatedBytes operator+(const AllocatedBytes& left, const AllocatedBytes& right);
} // namespace tarch

/**
 * The bytes an object occupies in host memory and in GPU memory. A build without a GPU backend
 * places device data in host memory, so all its bytes count as host bytes.
 */
struct tarch::AllocatedBytes final {
  std::size_t host{0};
  std::size_t device{0};

  [[nodiscard]] static INLINE_METHOD AllocatedBytes in(MemoryLocation location, std::size_t bytes);
  [[nodiscard]] static INLINE_METHOD AllocatedBytes onHost(std::size_t bytes);
  [[nodiscard]] INLINE_METHOD std::size_t getTotal() const;
  INLINE_METHOD AllocatedBytes& operator+=(const AllocatedBytes& other);
};

#include "tarch/accelerator/MemoryLocation.cpph"
