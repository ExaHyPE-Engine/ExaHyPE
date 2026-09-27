// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class ChunkedMemoryPoolTest;
} // namespace tarch::accelerator::tests

/**
 * Checks that a chunked memory pool hands out aligned chunks in whole segments and keeps the
 * chunks in place while it grows.
 */
class tarch::accelerator::tests::ChunkedMemoryPoolTest final : public tarch::tests::TestCase {
public:
  ChunkedMemoryPoolTest();
  ChunkedMemoryPoolTest(const ChunkedMemoryPoolTest&)            = delete;
  ChunkedMemoryPoolTest(ChunkedMemoryPoolTest&&)                 = delete;
  ChunkedMemoryPoolTest& operator=(const ChunkedMemoryPoolTest&) = delete;
  ChunkedMemoryPoolTest& operator=(ChunkedMemoryPoolTest&&)      = delete;
  ~ChunkedMemoryPoolTest() override                              = default;

  void run() override;

private:
  void testEmptyPool();
  void testReserveAllocatesWholeSegments();
  void testSegmentSizeFollowsTheFirstReservation();
  void testGrowthKeepsChunksInPlace();
  void testChunkAlignment();
  void testScalarTypes();
  void testViewAddressesEveryChunk();
  void testDeviceFillThroughView();
  void testAllocatedBytes();
  void testMoveTransfersSegments();
  void testRelease();
  void testHostCopiesRoundTrip();
};
