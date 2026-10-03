// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryBlocks.h"

#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Rank.h"

tarch::accelerator::MemoryBlocks&
tarch::accelerator::MemoryBlocks::getInstance() {
  static MemoryBlocks instance;
  return instance;
}

void
tarch::accelerator::MemoryBlocks::clear() {
  const std::scoped_lock lock(_memoryBlockMutex);
  _memoryBlockTable.clear();
}

void
tarch::accelerator::MemoryBlocks::checkForMemoryLeaks() {
#if PEANO_DEBUG
  const std::scoped_lock lock(_memoryBlockMutex);

  if (not _memoryBlockTable.empty()) {
    logWarning(
      "Detected " << _memoryBlockTable.size() << " memory leaks on rank " << tarch::mpi::Rank::getInstance().getRank()
                  << "."
    );
  } else {
    logInfo("Did not detect any memory leaks on rank " << tarch::mpi::Rank::getInstance().getRank() << ".");
  }

  for (const auto& [memoryBlock, size] : _memoryBlockTable) {
    logDebug("Memory block at " << memoryBlock << " contains " << size << " bytes.");
  }
#endif
}

void
tarch::accelerator::MemoryBlocks::bookmarkMemoryBlock(void* memoryBlock, std::size_t size) {
#if PEANO_DEBUG
  logDebug("Bookmarked memory block at " << memoryBlock << " with " << size << " bytes.");
  const std::scoped_lock lock(_memoryBlockMutex);
  _memoryBlockTable[memoryBlock] = size;
#endif
}

void
tarch::accelerator::MemoryBlocks::removeMemoryBlock(void* memoryBlock) {
#if PEANO_DEBUG
  const std::scoped_lock lock(_memoryBlockMutex);
  const auto             memoryBlockEntry = _memoryBlockTable.find(memoryBlock);
  if (memoryBlockEntry == _memoryBlockTable.end()) {
    tarch::terminateWithError("Attempt to remove an invalid memory block. It may have been removed already.");
  }
  logDebug("Removed memory block at " << memoryBlock << ".");
  _memoryBlockTable.erase(memoryBlockEntry);
#endif
}

/**
 * Returns the size of a bookmarked memory block, or zero outside a debug build. An unknown block
 * terminates a debug build.
 */
std::size_t
tarch::accelerator::MemoryBlocks::getSizeOfMemoryBlock(void* memoryBlock) const {
#if PEANO_DEBUG
  const std::scoped_lock lock(_memoryBlockMutex);
  const auto             memoryBlockEntry = _memoryBlockTable.find(memoryBlock);
  if (memoryBlockEntry == _memoryBlockTable.end()) {
    tarch::terminateWithError("Cannot query the size of an unregistered memory block.");
  }
  return memoryBlockEntry->second;
#else
  return 0;
#endif
}
