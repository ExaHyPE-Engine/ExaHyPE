// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridTraversalEventGeneratorTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridTraversalEventGeneratorTest final : public tarch::tests::TestCase {
private:
  void testCreateEnterNodeTraversalEvent_AllLocalPersistent();
  void testCreateEnterNodeTraversalEvent_VertexBoundaryImpliesLocalInFastPath();
  void testCreateEnterNodeTraversalEvent_InOutNoDataAndHasSplitOverride();
  void testCreateEnterNodeTraversalEvent_TemporaryStacksDontYieldNoData();
  void testCreateEnterNodeTraversalEvent_VertexStateSpecialCases();
  void testCreateEnterNodeTraversalEvent_CellNoDataAndHasSplitOverride();
  void testCreateLeaveNodeTraversalEvent_DeleteOnInOut();
  void testCreateLeaveNodeTraversalEvent_RecomputesHangingFacesAfterDFS();
  void testCreateLeaveNodeTraversalEvent_TemporaryStacksDontYieldNoData();

public:
  GridTraversalEventGeneratorTest();
  ~GridTraversalEventGeneratorTest() override = default;

  void run() override;
};
