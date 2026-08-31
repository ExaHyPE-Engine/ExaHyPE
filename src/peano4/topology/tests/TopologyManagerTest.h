// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::topology::tests {
  class TopologyManagerTest;
} // namespace peano4::topology::tests

class peano4::topology::tests::TopologyManagerTest final : public tarch::tests::TestCase {
public:
  TopologyManagerTest();
  TopologyManagerTest(const TopologyManagerTest&)            = delete;
  TopologyManagerTest(TopologyManagerTest&&)                 = delete;
  TopologyManagerTest& operator=(const TopologyManagerTest&) = delete;
  TopologyManagerTest& operator=(TopologyManagerTest&&)      = delete;
  ~TopologyManagerTest() override                            = default;

  void run() override;

private:
  void testTheSpawnLevelOffersEnoughOwnershipUnits();
  void testTheSpawnLevelStaysAboveTheLeaves();
  void testShallowGridsUseTheRootSpawnLevel();
  void testTheSpawnLevelGrowsWithTheRankCount();
  void testDeepGridsKeepTheirSpawnLevelInRange();
  void testNoTaskLoadStandsBeforeTheFirstSweep();
  void testTheRegridRealisationCoversEveryDelta();
  void testRejectedControlEventsChangeNoPlan();
  void testTheDomainAccessorsReturnTheInitialisedDomain();
};
