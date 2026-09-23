// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class ChunkedMemoryPoolTest;
} // namespace tarch::accelerator::tests

/**
 * Checks that a chunked memory pool hands out aligned chunks and keeps their
 * values across a resize.
 */
class tarch::accelerator::tests::ChunkedMemoryPoolTest final : public tarch::tests::TestCase {
public:
  ChunkedMemoryPoolTest();

  void run() override;

private:
  void testEmptyPool();
  void testResizeAndIndexedFill();
  void testResizeKeepsContent();
  void testFreshChunksReadAsZero();
  void testCapacityGrowsGeometrically();
  void testShrinkKeepsCapacity();
  void testChunkAlignment();
  void testScalarTypes();
  void testRawDataIsStrided();
  void testPinnedPlacement();
  void testAllocatedBytes();
  void testMoveTransfersBuffer();
  void testParallelDisjointFill();
};
