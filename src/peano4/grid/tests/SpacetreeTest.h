// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace peano4 {
  namespace grid {
    namespace tests {
      class SpacetreeTest;
    } // namespace tests
  } // namespace grid
} // namespace peano4


class peano4::grid::tests::SpacetreeTest final: public tarch::tests::TestCase {
private:
  void testRestrictToCoarseGrid();
  void testUpdateVertexRanksWithinCell();
  void testGetNeighbourTreesVertexSendReceive();
  void testGetNeighbourTreesFaceSendReceive();
  void testIsFaceAlongPeriodicBoundaryCondition();
  void testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags();
  void testIncrementNumberOfAdjacentRefinedLocalCells();
  void testDoesRankIndexIdentifyHorizontalDataExchange();
  void testDescend();
  void testDescendWhileInRunningState();

public:
  SpacetreeTest();
  ~SpacetreeTest() override final = default;

  void run() override final;
};
