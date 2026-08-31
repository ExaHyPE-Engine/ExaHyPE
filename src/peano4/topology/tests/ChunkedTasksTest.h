// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::topology::tests {
  class ChunkedTasksTest;
} // namespace peano4::topology::tests

class peano4::topology::tests::ChunkedTasksTest final : public tarch::tests::TestCase {
public:
  ChunkedTasksTest();
  ChunkedTasksTest(const ChunkedTasksTest&)            = delete;
  ChunkedTasksTest(ChunkedTasksTest&&)                 = delete;
  ChunkedTasksTest& operator=(const ChunkedTasksTest&) = delete;
  ChunkedTasksTest& operator=(ChunkedTasksTest&&)      = delete;
  ~ChunkedTasksTest() override                         = default;

  void run() override;

private:
  void testChunksCoverTheRangeOnceInOrder();
  void testNumberOfChunksFollowsTheMinimum();
  void testASmallRangeRunsInOneChunk();
  void testAnEmptyRangeRunsOnce();
};
