// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChunkedTasks.h"

#include "tarch/Assertions.h"
#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/TaskGraph.h"

/**
 * Returns a strict upper bound of the chunk count of runChunkedTasks().
 */
int
peano4::topology::getMaximumNumberOfChunks() {
  return 2 * std::max(tarch::tasking::Runtime::getInstance().getNumberOfThreads(), 1) + 1;
}

/**
 * Returns the number of chunks for runChunkedTasks(). There are at most two per thread and at least
 * minimumItemsPerChunk items per chunk, or one chunk for a shorter range.
 */
int
peano4::topology::getNumberOfChunks(const int numberOfItems, const int minimumItemsPerChunk) {
  ASSERTION(numberOfItems >= 0, numberOfItems);
  ASSERTION(minimumItemsPerChunk > 0, minimumItemsPerChunk);
  const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  if (numberOfThreads <= 1) {
    return 1;
  }
  return std::min(2 * numberOfThreads, std::max(1, numberOfItems / minimumItemsPerChunk));
}

/**
 * Runs processRange(chunkNumber, begin, end) as one task per chunk, or in the calling thread for a
 * single chunk. The chunk number stays below getMaximumNumberOfChunks(). The chunks cover the range
 * in order and per-chunk buffers concatenate to the order of a serial loop.
 */
void
peano4::topology::runChunkedTasks(
  const int                                 numberOfItems,
  const std::function<void(int, int, int)>& processRange,
  const int                                 minimumItemsPerChunk
) {
  const int numberOfChunks = peano4::topology::getNumberOfChunks(numberOfItems, minimumItemsPerChunk);
  ASSERTION(numberOfChunks >= 1 and numberOfChunks < peano4::topology::getMaximumNumberOfChunks(), numberOfChunks);
  if (numberOfChunks <= 1) {
    processRange(0, 0, numberOfItems);
    return;
  }
  std::vector<char>  completionTokens(numberOfChunks);
  std::vector<void*> tokenAddresses(numberOfChunks);
  for (int chunkNumber = 0; chunkNumber < numberOfChunks; chunkNumber++) {
    const int begin = static_cast<int>(static_cast<std::int64_t>(numberOfItems) * chunkNumber / numberOfChunks);
    const int end   = static_cast<int>(static_cast<std::int64_t>(numberOfItems) * (chunkNumber + 1) / numberOfChunks);
    tokenAddresses[chunkNumber] = &completionTokens[chunkNumber];
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([chunkNumber, begin, end, &processRange]() {
        processRange(chunkNumber, begin, end);
      }),
      {},
      {tokenAddresses[chunkNumber]}
    );
  }
  tarch::tasking::waitForDependencies(tokenAddresses);
}
