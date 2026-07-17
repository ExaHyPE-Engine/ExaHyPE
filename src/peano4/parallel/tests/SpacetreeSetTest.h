// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class SpacetreeSetTest;
} // namespace peano4::parallel::tests

class peano4::parallel::tests::SpacetreeSetTest final : public tarch::tests::TestCase {
public:
  SpacetreeSetTest();
  void run() override;

private:
  /** Aggregates one scenario over its root and every descendant tree. */
  struct ScenarioResult final {
    int              trees          = 0;
    int              unrefinedCells = 0;
    int              refinedCells   = 0;
    int              maxDepth       = 0;
    std::vector<int> unrefinedCellsPerTree;
    bool             routingResolved   = false;
    bool             routingIsSelfOnly = false;
    bool             splitsSucceeded   = true;
  };

  ScenarioResult runScenario(bool periodic, int targetTrees, bool refineCorner, bool eraseCornerAgain) const;

  void testRegularGridLoadBalancingMatchesNonPeriodicRun();
  void testCornerRefinementMirrorsAcrossPeriodicBoundaries();
  void testMultiTreePeriodicGridMatchesSingleTree();
  void testEraseAtPeriodicBoundaryRestoresRegularGrid();
  void testWorkerCopiesRequestedMasterState();
  void testRemoteSplitReservesIdsAsOneBatch();
  void testDistributedSplitLifecycle();
  void testCleanupRemovesConsecutiveTrees();
  void testAggregateStatisticsIncludesSettlingTrees();
};
