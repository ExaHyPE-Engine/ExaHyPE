// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class VertexExtremaTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks how the observable extrema of the cells reach, widen and merge at a vertex.
 */
class exahype2::dg::limiting::tests::VertexExtremaTest final : public tarch::tests::TestCase {
public:
  VertexExtremaTest();
  VertexExtremaTest(const VertexExtremaTest&)            = delete;
  VertexExtremaTest(VertexExtremaTest&&)                 = delete;
  VertexExtremaTest& operator=(const VertexExtremaTest&) = delete;
  VertexExtremaTest& operator=(VertexExtremaTest&&)      = delete;
  ~VertexExtremaTest() override                          = default;

  void run() override;

private:
  void testResetSeedsInfinities();
  void testCellExtremaReachEveryAdjacentVertex();
  void testVertexExtremaWidenTheCellRange();
  void testHangingVertexTakesTheExtremaOfItsFacetVertices();
  void testHangingVertexExtremaReachTheFacetVertices();
  void testMergeReducesTwoCopiesOfOneVertex();
};
