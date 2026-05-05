// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <map>

namespace tarch::accelerator {
  class MemoryBlocks;
} // namespace tarch::accelerator

class tarch::accelerator::MemoryBlocks {
private:
  std::map<void*, std::size_t> _memoryBlockTable;
  multicore::BooleanSemaphore  _memoryBlockSemaphore;

  MemoryBlocks() = default;

public:
  ~MemoryBlocks() = default;

  static MemoryBlocks& getInstance();

  void        clear();
  void        checkForMemoryLeaks();
  void        bookmarkMemoryBlock(void* memoryBlock, std::size_t size);
  void        removeMemoryBlock(void* memoryBlock);
  std::size_t getSizeOfMemoryBlock(void* memoryBlock) const;
};
