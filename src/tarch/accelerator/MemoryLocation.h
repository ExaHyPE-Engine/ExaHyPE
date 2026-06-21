// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

namespace tarch {
  enum class MemoryLocation {
    /**
     * Create data on the heap of the host.
     *
     * The alignment of this routine can be controlled via the compile
     * time constant PEANO_WITH_ALIGNMENT. It is typically set (to a default)
     * within the corresponding compiler-specific header-file in
     * tarch/compiler.
     */
    Heap,

    /**
     * Create data on the device.
     */
    Device,

    Pinned,
  };

  std::string toString(MemoryLocation value);
} // namespace tarch
