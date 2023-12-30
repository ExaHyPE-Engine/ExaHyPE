// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::memory {
  enum class MemoryLocation {
    /**
     * Create data on the heap of the local device. That is, if you run
     * on an accelerator, use the accelerator's heap. If you invoke it
     * on the host, use the host's accelerator.
     *
     * The alignment of this routine can be controlled via the compile
     * time constant AlignmentOnHeap. It is typically set (to a default)
     * within the corresponding compiler-specific header-file in
     * tarch/compiler.
     */
    Heap,

    /**
     * Create data using USM managed memory.
     */
    ManagedSharedAcceleratorDeviceMemory,

    PinnedMemory,
  };
} // namespace tarch::memory

std::string toString(MemoryLocation value);
