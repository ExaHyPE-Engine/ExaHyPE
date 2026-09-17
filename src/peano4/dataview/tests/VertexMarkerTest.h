// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::dataview::tests {
  class VertexMarkerTest;
} // namespace peano4::dataview::tests

class peano4::dataview::tests::VertexMarkerTest final : public tarch::tests::TestCase {
public:
  VertexMarkerTest();

  void run() override;

private:
  void testSelection();
  void testGeometry();
  void testStateBits();
  void testRelativePositionWithinFatherCell();
  void testCoincidesWithCoarseGridVertex();
  void testContainment();
  void testAdjacency();
  void testReconstructXOfParentVertex();
};
