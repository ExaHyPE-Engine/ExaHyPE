// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridControlEventTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridControlEventTest final : public tarch::tests::TestCase {
private:
  void testOperatorEquals();
  void testOperatorLess();
  void testSort();
  void testSortDeterministicGrid();
  void testMergeExactTouchVersusGap();
  void testMergeAdjacency();
  void testMergeEraseAdjacency();
  void testMergeEraseDoesNotOverMergeLShape();
  void testMergeDoesNotChangeGridDecisions();
  void testMergeEraseChainCollapsesToOne();
  void testMergeEraseSiblingGroup();
  void testMergeKeepsRefineAndEraseSeparate();
  void testMergeRowAndBlockCollapseToOne();
  void testMergeDoesNotMergeDiagonalOrDifferentTargetH();
  void testMergeUnionIsPreserved();
  void testMerge1();
  void testMerge2();
  void testMerge3();
  void testMPI();

public:
  GridControlEventTest();
  ~GridControlEventTest() override = default;

  void run() override;
};
