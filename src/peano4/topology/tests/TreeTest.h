// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::topology::tests {
  class TreeTest;
} // namespace peano4::topology::tests

class peano4::topology::tests::TreeTest final : public tarch::tests::TestCase {
public:
  TreeTest();
  TreeTest(const TreeTest&)            = delete;
  TreeTest(TreeTest&&)                 = delete;
  TreeTest& operator=(const TreeTest&) = delete;
  TreeTest& operator=(TreeTest&&)      = delete;
  ~TreeTest() override                 = default;

  void run() override;

private:
  void testRegularGridSizes();
  void testCellLinks();
  void testDenseCellLookupMatchesTheLattice();
  void testAdjacentCellsReferenceOneVertex();
  void testVertexContent();
  void testRefinementPlanDilation();
  void testRefinementPlanCoarseDilation();
  void testRefinementPlanWrapsOnAPeriodicAxis();
  void testRefinementPlanErase();
  void testRefinementPlanRoundKeepsLevelsWithoutRequests();
  void testRefinementPlanEraseKeepsFlagsBetweenRefinedCells();
  void testVertexBetweenRefinedCellsIsPersistent();
  void testAppendedDeltaMatchesADirectBuild();
  void testPlanViewFromDeltaMatchesADirectBuild();
  void testBalancedOwnersFromPlan();
  void testTheBalancedPartitionKeepsBaseFamiliesWhole();
  void testRankLoadsOfADeeperOwnershipPlan();
  void testAPartitionSplittingALeafFamilyIsDetected();
  void testARankWithoutABlockMaterialisesNothingBelowTheSpawnLevel();
  void testAPartitionedBuildLinksEveryEntityOnce();
};
