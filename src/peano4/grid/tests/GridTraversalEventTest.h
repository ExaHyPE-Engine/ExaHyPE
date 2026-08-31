// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridTraversalEventTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridTraversalEventTest final : public tarch::tests::TestCase {
public:
  GridTraversalEventTest();
  GridTraversalEventTest(const GridTraversalEventTest&)            = delete;
  GridTraversalEventTest(GridTraversalEventTest&&)                 = delete;
  GridTraversalEventTest& operator=(const GridTraversalEventTest&) = delete;
  GridTraversalEventTest& operator=(GridTraversalEventTest&&)      = delete;
  ~GridTraversalEventTest() override                               = default;

  void run() override;

private:
  void testDefaults();
  void testBitHelpers();
  void testGeometry();
  void testVertexMasks();
  void testFaceMasks();
  void testCellFlags();
  void testDataIndices();
  void testToString();
  void testCellMarkerCarriesTheCellFlags();
  void testFaceMarkerCarriesTheFaceAndTheCellFlags();
};
