// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class SpacetreeTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::SpacetreeTest final : public tarch::tests::TestCase {
public:
  SpacetreeTest();
  void run() override;

private:
  void testRestrictToCoarseGrid();
  void testUpdateVertexRanksWithinCell();
  void testGetNeighbourTreesVertexSendReceive();
  void testGetNeighbourTreesFaceSendReceive();
  void testIsFaceAlongPeriodicBoundaryCondition();
  void testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags();
  void testIncrementNumberOfAdjacentRefinedLocalCells();
  void testDoesRankIndexIdentifyHorizontalDataExchange();
  void testHorizontalExchangeSymmetry();
  void testNoTopologyChangeParity();
  void testVertexNeighbourPermutationInvariance();
  void testDescend();
  void testSplittingTreeCursorSkipsCompletedRequests();
  void testSplittingTreeCursorResetsAcrossTraversal();
  void testSplitModeNormalisationUsesLowestTreeIdMode();
};
