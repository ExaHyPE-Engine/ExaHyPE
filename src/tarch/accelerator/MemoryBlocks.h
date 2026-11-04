// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::accelerator {
  class MemoryBlocks;
} // namespace tarch::accelerator

class tarch::accelerator::MemoryBlocks final {
public:
  MemoryBlocks(const MemoryBlocks&)            = delete;
  MemoryBlocks(MemoryBlocks&&)                 = delete;
  MemoryBlocks& operator=(const MemoryBlocks&) = delete;
  MemoryBlocks& operator=(MemoryBlocks&&)      = delete;
  ~MemoryBlocks()                              = default;

  static MemoryBlocks& getInstance();

  void clear();
  void checkForMemoryLeaks();
  void bookmarkMemoryBlock(void* memoryBlock, std::size_t size);
  void removeMemoryBlock(void* memoryBlock);
  std::size_t getSizeOfMemoryBlock(void* memoryBlock) const;

private:
  std::map<void*, std::size_t> _memoryBlockTable{};
  mutable std::mutex           _memoryBlockMutex{};

  MemoryBlocks() = default;
};
