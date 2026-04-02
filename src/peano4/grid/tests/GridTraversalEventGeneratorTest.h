// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace peano4 {
  namespace grid {
    namespace tests {
      class GridTraversalEventGeneratorTest;
    } // namespace tests
  } // namespace grid
} // namespace peano4

class peano4::grid::tests::GridTraversalEventGeneratorTest final: public tarch::tests::TestCase {
private:
  void testCreateEnterNodeTraversalEvent_AllLocalPersistent();
  void testCreateEnterNodeTraversalEvent_VertexBoundaryImpliesLocalInFastPath();
  void testCreateEnterNodeTraversalEvent_InOutNoDataAndHasSplitOverride();
  void testCreateEnterNodeTraversalEvent_TemporaryStacksDontYieldNoData();
  void testCreateEnterNodeTraversalEvent_VertexStateSpecialCases();
  void testCreateEnterNodeTraversalEvent_CellNoDataAndHasSplitOverride();
  void testCreateLeaveNodeTraversalEvent_DeleteOnInOut();
  void testCreateLeaveNodeTraversalEvent_TemporaryStacksDontYieldNoData();

public:
  GridTraversalEventGeneratorTest();
  ~GridTraversalEventGeneratorTest() override final = default;

  void run() override final;
};
