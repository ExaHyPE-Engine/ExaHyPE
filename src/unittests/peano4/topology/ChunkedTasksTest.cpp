// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/topology/ChunkedTasks.h"

#include "tarch/tasking/Runtime.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("Chunked tasks visit every item of a range once, in contiguous ascending chunks below the chunk bound") {
  GIVEN("A range of 10000 items and the chunk minimum 64") {
    constexpr int NumberOfItems        = 10000;
    constexpr int MinimumItemsPerChunk = 64;

    WHEN("The range runs as chunked tasks and every chunk counts the visits of its items and stores its range") {
      const int                        maximumNumberOfChunks = peano4::topology::getMaximumNumberOfChunks();
      std::vector<std::atomic<int>>    visits(NumberOfItems);
      std::vector<std::pair<int, int>> rangeOfChunk(maximumNumberOfChunks, {-1, -1});
      std::atomic<int>                 numberOfChunks{0};
      std::atomic<int>                 chunksBeyondTheBound{0};
      peano4::topology::runChunkedTasks(
        NumberOfItems,
        [&](const int chunkNumber, const int begin, const int end) {
          if (chunkNumber < 0 or chunkNumber >= maximumNumberOfChunks) {
            chunksBeyondTheBound++;
            return;
          }
          rangeOfChunk[chunkNumber] = {begin, end};
          numberOfChunks++;
          for (int itemNumber = begin; itemNumber < end; itemNumber++) {
            visits[itemNumber]++;
          }
        },
        MinimumItemsPerChunk
      );

      THEN("Every chunk number stays below the bound and the number of chunks follows the chunk minimum") {
        VALIDATE_EQUALS(chunksBeyondTheBound.load(), 0, maximumNumberOfChunks);
        VALIDATE_EQUALS(
          numberOfChunks.load(),
          peano4::topology::getNumberOfChunks(NumberOfItems, MinimumItemsPerChunk)
        );
      }

      THEN("Every item receives one visit and the chunks tile the range in ascending order") {
        for (int itemNumber = 0; itemNumber < NumberOfItems; itemNumber++) {
          VALIDATE_EQUALS(visits[itemNumber].load(), 1, itemNumber);
        }
        int expectedBegin = 0;
        for (int chunkNumber = 0; chunkNumber < numberOfChunks; chunkNumber++) {
          VALIDATE_EQUALS(rangeOfChunk[chunkNumber].first, expectedBegin, chunkNumber);
          VALIDATE(rangeOfChunk[chunkNumber].second > rangeOfChunk[chunkNumber].first, chunkNumber);
          expectedBegin = rangeOfChunk[chunkNumber].second;
        }
        VALIDATE_EQUALS(expectedBegin, NumberOfItems);
      }
    }
  }
}

UNIT_TEST("The number of chunks grows from one for a short range to two per thread for a long range") {
  GIVEN("The default chunk minimum and the chunk counts of two minimums and of a long range for the running threads") {
    constexpr int MinimumItemsPerChunk = peano4::topology::DefaultMinimumItemsPerChunk;
    const int     numberOfThreads      = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int     chunksOfTwoMinimums  = numberOfThreads > 1 ? 2 : 1;
    const int     chunksOfLongRange    = numberOfThreads > 1 ? 2 * numberOfThreads : 1;

    WHEN("The test counts the chunks of an empty range, of ranges below and at two minimums and of 1000 minimums") {
      const int chunksOfEmptyRange       = peano4::topology::getNumberOfChunks(0);
      const int chunksBelowTwoMinimums   = peano4::topology::getNumberOfChunks(2 * MinimumItemsPerChunk - 1);
      const int chunksAtTwoMinimums      = peano4::topology::getNumberOfChunks(2 * MinimumItemsPerChunk);
      const int chunksAtTwoSmallMinimums = peano4::topology::getNumberOfChunks(64, 32);
      const int chunksOfThousandMinimums = peano4::topology::getNumberOfChunks(1000 * MinimumItemsPerChunk);

      THEN("Every count matches its range and the running threads and the long range stays below the chunk bound") {
        VALIDATE_EQUALS(chunksOfEmptyRange, 1);
        VALIDATE_EQUALS(chunksBelowTwoMinimums, 1);
        VALIDATE_EQUALS(chunksAtTwoMinimums, chunksOfTwoMinimums);
        VALIDATE_EQUALS(chunksAtTwoSmallMinimums, chunksOfTwoMinimums);
        VALIDATE_EQUALS(chunksOfThousandMinimums, chunksOfLongRange);
        VALIDATE(chunksOfLongRange < peano4::topology::getMaximumNumberOfChunks(), chunksOfLongRange);
      }
    }
  }
}

UNIT_TEST("A range below two chunk minimums runs as one chunk in the calling thread") {
  GIVEN("A range of 100 items and the number of the calling thread") {
    constexpr int NumberOfItems = 100;
    const int     callingThread = tarch::tasking::Runtime::getInstance().getThreadNumber();

    WHEN("The range runs as chunked tasks and every chunk stores its number, its range and its thread") {
      std::vector<int> chunkNumbers;
      int              firstBegin  = -1;
      int              firstEnd    = -1;
      int              chunkThread = -1;
      peano4::topology::runChunkedTasks(NumberOfItems, [&](const int chunkNumber, const int begin, const int end) {
        chunkNumbers.push_back(chunkNumber);
        firstBegin  = begin;
        firstEnd    = end;
        chunkThread = tarch::tasking::Runtime::getInstance().getThreadNumber();
      });

      THEN("One chunk with the number zero covers the whole range in the calling thread") {
        for (const int chunkNumber : chunkNumbers) {
          VALIDATE_EQUALS(chunkNumber, 0);
        }
        VALIDATE_EQUALS(static_cast<int>(chunkNumbers.size()), 1);
        VALIDATE_EQUALS(firstBegin, 0);
        VALIDATE_EQUALS(firstEnd, NumberOfItems);
        VALIDATE_EQUALS(chunkThread, callingThread);
      }
    }
  }
}

UNIT_TEST("An empty range calls the chunk body once with an empty range") {
  GIVEN("A range without items") {
    constexpr int NumberOfItems = 0;

    WHEN("The range runs as chunked tasks and every chunk stores its number and counts its items") {
      std::vector<int> chunkNumbers;
      int              itemsSeen = 0;
      peano4::topology::runChunkedTasks(NumberOfItems, [&](const int chunkNumber, const int begin, const int end) {
        chunkNumbers.push_back(chunkNumber);
        itemsSeen += end - begin;
      });

      THEN("One chunk with the number zero runs over zero items") {
        for (const int chunkNumber : chunkNumbers) {
          VALIDATE_EQUALS(chunkNumber, 0);
        }
        VALIDATE_EQUALS(static_cast<int>(chunkNumbers.size()), 1);
        VALIDATE_EQUALS(itemsSeen, 0);
      }
    }
  }
}
