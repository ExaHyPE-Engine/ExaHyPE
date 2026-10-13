// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class SpacetreeTest;
} // namespace peano4::tests

/**
 * Checks the spawn level of the spacetree and the regrids of its singleton.
 */
class peano4::tests::SpacetreeTest final : public tarch::tests::TestCase {
public:
  SpacetreeTest();
  SpacetreeTest(const SpacetreeTest&)            = delete;
  SpacetreeTest(SpacetreeTest&&)                 = delete;
  SpacetreeTest& operator=(const SpacetreeTest&) = delete;
  SpacetreeTest& operator=(SpacetreeTest&&)      = delete;
  ~SpacetreeTest() override                      = default;

  void run() override;

private:
  void testTheSpawnLevelOffersEnoughOwnershipUnits();
  void testTheSpawnLevelStaysAboveTheCells();
  void testShallowGridsUseTheRootSpawnLevel();
  void testTheSpawnLevelGrowsWithTheRankCount();
  void testDeepGridsKeepTheirSpawnLevelInRange();
  void testNoStatisticsStandBeforeTheFirstRegrid();
  void testARankWithoutCellsReportsNoLevel();
  void testRejectedRequestsChangeNoPlan();
  void testARegridWithLiveDataReturnsTheSlotMoves();
  void testTheTreeBytesCountTheSpareTree();
  void testTheCellConnectivityBytesCountTheFreedSlots();
  void testTheTreeBytesCountTheEvaluatedPlan();
  void testTheCellLookupFindsEveryCellAndNothingElse();
  void testThePeriodicBoundariesReturnTheInitialisedValue();
};
