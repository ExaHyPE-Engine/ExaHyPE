// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace peano4 {
  namespace grid {
    namespace tests {
      class SpacetreeHelperTest;
    } // namespace tests
  } // namespace grid
} // namespace peano4

class peano4::grid::tests::SpacetreeHelperTest final: public tarch::tests::TestCase {
private:
  void testGetAdjacentRanksOfFace_SendVsReceiveAndStateFiltering();
  void testAreFacesLocal_HangingVertexRules();
  void testParallelDomainBoundaryAdjacencyFlags();
  void testPatch3x3IndexTable();
  void testFaceAdjacencyTables();
  void testAreAllVerticesNonHanging();
  void testIsSpacetreeNodeLocal();
  void testGetTreeOwningSpacetreeNode();
  void testIsSpacetreeNodeSplitCandidate();

public:
  SpacetreeHelperTest();
  ~SpacetreeHelperTest() override final = default;

  void run() override final;
};
