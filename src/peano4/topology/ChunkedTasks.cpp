// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChunkedTasks.h"

#include "tarch/Assertions.h"
#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/TaskGraph.h"

/** Returns the upper bound on the chunk count runChunkedTasks() uses. */
int peano4::topology::maximumNumberOfChunks() {
  return 2 * std::max(tarch::tasking::Runtime::getInstance().getNumberOfThreads(), 1) + 1;
}

/**
 * Returns the number of chunks runChunkedTasks() cuts a range into. The caller
 * states the item count one chunk has to carry, and a range below two of them
 * runs as one chunk. Two chunks per thread let the runtime balance chunks of
 * unequal cost.
 */
int peano4::topology::numberOfChunks(const int numberOfItems, const int minimumItemsPerChunk) {
  assertion(numberOfItems >= 0, numberOfItems);
  assertion(minimumItemsPerChunk > 0, minimumItemsPerChunk);
  const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  if (numberOfThreads <= 1) {
    return 1;
  }
  return std::min(2 * numberOfThreads, std::max(1, numberOfItems / minimumItemsPerChunk));
}

/**
 * Runs a loop over an index range as one task per chunk. numberOfChunks()
 * decides the cut, and a single chunk runs in the calling thread. The callback
 * receives its chunk number, which stays below maximumNumberOfChunks(), so it
 * can accumulate into a buffer of its own, and a concatenation of those buffers
 * in chunk order reproduces the order of a serial loop.
 */
void peano4::topology::runChunkedTasks(
  const int                                 numberOfItems,
  const std::function<void(int, int, int)>& processRange,
  const int                                 minimumItemsPerChunk
) {
  const int numberOfChunks = peano4::topology::numberOfChunks(numberOfItems, minimumItemsPerChunk);
  assertion(numberOfChunks >= 1 and numberOfChunks < peano4::topology::maximumNumberOfChunks(), numberOfChunks);
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
