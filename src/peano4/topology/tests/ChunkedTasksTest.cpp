// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChunkedTasksTest.h"

#include "peano4/topology/ChunkedTasks.h"
#include "tarch/tasking/Runtime.h"

peano4::topology::tests::ChunkedTasksTest::ChunkedTasksTest() :
  TestCase("peano4::topology::tests::ChunkedTasksTest") {}

void
peano4::topology::tests::ChunkedTasksTest::run() {
  testMethod(testChunksCoverTheRangeOnceInOrder);
  testMethod(testNumberOfChunksFollowsTheMinimum);
  testMethod(testASmallRangeRunsInOneChunk);
  testMethod(testAnEmptyRangeRunsOnce);
}

/**
 * Every item of a range above the chunk minimum lands in exactly one chunk,
 * the chunks are contiguous and ascending, and their number stays below the
 * bound getMaximumNumberOfChunks() names.
 */
void
peano4::topology::tests::ChunkedTasksTest::testChunksCoverTheRangeOnceInOrder() {
  constexpr int NumberOfItems        = 10000;
  constexpr int MinimumItemsPerChunk = 64;

  std::vector<std::atomic<int>>    visits(NumberOfItems);
  std::vector<std::pair<int, int>> rangeOfChunk(peano4::topology::getMaximumNumberOfChunks(), {-1, -1});
  std::atomic<int>                 numberOfChunks{0};
  peano4::topology::runChunkedTasks(
    NumberOfItems,
    [&](const int chunkNumber, const int begin, const int end) {
      validateWithParams1(chunkNumber >= 0 and chunkNumber < peano4::topology::getMaximumNumberOfChunks(), chunkNumber);
      rangeOfChunk[chunkNumber] = {begin, end};
      numberOfChunks++;
      for (int itemNumber = begin; itemNumber < end; itemNumber++) {
        visits[itemNumber]++;
      }
    },
    MinimumItemsPerChunk
  );

  for (int itemNumber = 0; itemNumber < NumberOfItems; itemNumber++) {
    validateEqualsWithParams1(visits[itemNumber].load(), 1, itemNumber);
  }
  int expectedBegin = 0;
  for (int chunkNumber = 0; chunkNumber < numberOfChunks; chunkNumber++) {
    validateEqualsWithParams1(rangeOfChunk[chunkNumber].first, expectedBegin, chunkNumber);
    validateWithParams1(rangeOfChunk[chunkNumber].second > rangeOfChunk[chunkNumber].first, chunkNumber);
    expectedBegin = rangeOfChunk[chunkNumber].second;
  }
  validateEquals(expectedBegin, NumberOfItems);
  const int expectedNumberOfChunks = peano4::topology::getNumberOfChunks(NumberOfItems, MinimumItemsPerChunk);
  const int actualNumberOfChunks   = numberOfChunks.load();
  validateEquals(actualNumberOfChunks, expectedNumberOfChunks);
}

/**
 * A range below two minimum chunks is one chunk, a range of two minimum chunks
 * is two, and a long range cuts into two chunks per thread, below the bound
 * getMaximumNumberOfChunks() names.
 */
void
peano4::topology::tests::ChunkedTasksTest::testNumberOfChunksFollowsTheMinimum() {
  constexpr int MinimumItemsPerChunk = peano4::topology::DefaultMinimumItemsPerChunk;
  const int     numberOfThreads      = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int     chunksOfTwoMinimums  = numberOfThreads > 1 ? 2 : 1;
  const int     chunksOfLongRange    = numberOfThreads > 1 ? 2 * numberOfThreads : 1;
  validateEquals(peano4::topology::getNumberOfChunks(0), 1);
  validateEquals(peano4::topology::getNumberOfChunks(2 * MinimumItemsPerChunk - 1), 1);
  validateEquals(peano4::topology::getNumberOfChunks(2 * MinimumItemsPerChunk), chunksOfTwoMinimums);
  validateEquals(peano4::topology::getNumberOfChunks(64, 32), chunksOfTwoMinimums);
  validateEquals(peano4::topology::getNumberOfChunks(1000 * MinimumItemsPerChunk), chunksOfLongRange);
  validateWithParams1(chunksOfLongRange < peano4::topology::getMaximumNumberOfChunks(), chunksOfLongRange);
}

/** A range below two chunk minimums runs as one chunk in the calling thread. */
void
peano4::topology::tests::ChunkedTasksTest::testASmallRangeRunsInOneChunk() {
  constexpr int NumberOfItems = 100;

  int       numberOfChunks = 0;
  int       firstBegin     = -1;
  int       firstEnd       = -1;
  const int callingThread  = tarch::tasking::Runtime::getInstance().getThreadNumber();
  int       chunkThread    = -1;
  peano4::topology::runChunkedTasks(NumberOfItems, [&](const int chunkNumber, const int begin, const int end) {
    validateEquals(chunkNumber, 0);
    numberOfChunks++;
    firstBegin  = begin;
    firstEnd    = end;
    chunkThread = tarch::tasking::Runtime::getInstance().getThreadNumber();
  });
  validateEquals(numberOfChunks, 1);
  validateEquals(firstBegin, 0);
  validateEquals(firstEnd, NumberOfItems);
  validateEquals(chunkThread, callingThread);
}

/** An empty range calls the chunk body once with an empty range. */
void
peano4::topology::tests::ChunkedTasksTest::testAnEmptyRangeRunsOnce() {
  int numberOfChunks = 0;
  int itemsSeen      = 0;
  peano4::topology::runChunkedTasks(0, [&](const int chunkNumber, const int begin, const int end) {
    validateEquals(chunkNumber, 0);
    numberOfChunks++;
    itemsSeen += end - begin;
  });
  validateEquals(numberOfChunks, 1);
  validateEquals(itemsSeen, 0);
}
