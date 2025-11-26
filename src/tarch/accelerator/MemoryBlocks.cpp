// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryBlocks.h"

#include "tarch/mpi/Rank.h"
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
#if PEANO_DEBUG >= 2
  multicore::Lock lock(_memoryBlockSemaphore);

  if (not _memoryBlockTable.empty()) {
    logWarning(
      "checkForMemoryLeaks()",
      "Detected "
        << _memoryBlockTable.size() << " memory leaks on rank " << tarch::mpi::Rank::getInstance().getRank() << "!"
    );
  } else {
    logInfo(
      "checkForMemoryLeaks()",
      "Did not detect any memory leaks on rank " << tarch::mpi::Rank::getInstance().getRank() << "."
    );
  }

  auto itr = _memoryBlockTable.begin();
  auto end = _memoryBlockTable.end();

  for (; itr != end; ++itr) {
    logDebug("checkForMemoryLeaks()", "Memory block @" << itr->first << " with size: " << itr->second);
  }
#else
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo(
      "checkForMemoryLeaks()",
      "Have not bookmarked any memory blocks. Recompile in DEBUG mode to check for memory leaks."
    );
  }
#endif
}

void tarch::accelerator::MemoryBlocks::bookmarkMemoryBlock(void* memoryBlock, std::size_t size) {
#if PEANO_DEBUG >= 2
  logDebug("bookmarkMemoryBlock()", "Bookmark memory block @" << memoryBlock << " with size: " << size);
  multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable[memoryBlock] = size;
#endif
}

void tarch::accelerator::MemoryBlocks::removeMemoryBlock(void* memoryBlock) {
#if PEANO_DEBUG >= 2
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
#if PEANO_DEBUG >= 2
  return _memoryBlockTable.at(memoryBlock);
#else
  return 0;
#endif
}
