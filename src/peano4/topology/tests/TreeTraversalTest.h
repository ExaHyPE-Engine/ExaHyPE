// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::topology::tests {
  class TreeTraversalTest;
} // namespace peano4::topology::tests

class peano4::topology::tests::TreeTraversalTest final : public tarch::tests::TestCase {
public:
  TreeTraversalTest();
  TreeTraversalTest(const TreeTraversalTest&)            = delete;
  TreeTraversalTest(TreeTraversalTest&&)                 = delete;
  TreeTraversalTest& operator=(const TreeTraversalTest&) = delete;
  TreeTraversalTest& operator=(TreeTraversalTest&&)      = delete;
  ~TreeTraversalTest() override                          = default;

  void run() override;

private:
  void testRootChildrenBecomeTasksAtSpawnLevelZero();
  void testFreshSlotsNumberTheEntitiesInIndexOrder();
  void testRegridTransitionClasses();
  void testTransitionTasksRootAtChangingCells();
  void testSlotInheritanceAcrossRegrid();
  void testShrunkInterfaceTopologyKeepsTheSurvivingSlots();
  void testRankPairsAgreeOnTheirBoundaryWithoutCommunicating();
  void testTransferListsPairAcrossRanksWhenThePartitionMoves();
  void testInPlaceRegridRederivesTheRankBoundary();
  void testRemoteBlockRootsMoveTheVerticesTheRankHolds();
  void testVerticesCountOnceAcrossRanks();
};
