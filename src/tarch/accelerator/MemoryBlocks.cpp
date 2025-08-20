// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryBlocks.h"

#include "tarch/multicore/Lock.h"

tarch::logging::Log tarch::accelerator::MemoryBlocks::_log("tarch::accelerator::MemoryBlocks");

tarch::accelerator::MemoryBlocks& tarch::accelerator::MemoryBlocks::getInstance() {
  static MemoryBlocks instance;
  return instance;
}

void tarch::accelerator::MemoryBlocks::clear() {
  multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
}

void tarch::accelerator::MemoryBlocks::checkForMemoryLeaks() {
#if 0
  multicore::Lock lock(_memoryBlockSemaphore);

  logInfo("checkForMemoryLeaks()", "Checking if any memory leaks occurred ...");

  if (!_memoryBlockTable.empty()) {
    logWarning("checkForMemoryLeaks()", "Detected " << _memoryBlockTable.size() << " memory leaks!");
  } else {
    logInfo("checkForMemoryLeaks()", "All is fine");
  }

  auto itr = _memoryBlockTable.begin();
  auto end = _memoryBlockTable.end();

  for (; itr != end; ++itr) {
    logDebug("checkForMemoryLeaks()", "Memory block @" << itr->first << " with size: " << itr->second);
  }
#endif
}

void tarch::accelerator::MemoryBlocks::bookmarkMemoryBlock(void* memoryBlock, std::size_t size) {
#if 0
  logDebug("bookmarkMemoryBlock()", "Bookmark memory block @" << memoryBlock << " with size: " << size);
  multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable[memoryBlock] = size;
#endif
}

void tarch::accelerator::MemoryBlocks::removeMemoryBlock(void* memoryBlock) {
#if 0
  multicore::Lock lock(_memoryBlockSemaphore);
  auto            itr = _memoryBlockTable.find(memoryBlock);
  if (itr == _memoryBlockTable.end()) {
    logError(
      "removeMemoryBlock()",
      "Attempt to remove invalid memory block. Perhaps it has been removed before? Abort."
    );
    std::abort();
  }
  logDebug("removeMemoryBlock()", "Remove memory block @" << memoryBlock);
  _memoryBlockTable.erase(itr);
#endif
}

std::size_t tarch::accelerator::MemoryBlocks::getSizeOfMemoryBlock(void* memoryBlock) const {
  return _memoryBlockTable.at(memoryBlock);
}
