// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::dataview::tests {
  class FaceMarkerTest;
} // namespace peano4::dataview::tests

class peano4::dataview::tests::FaceMarkerTest final : public tarch::tests::TestCase {
public:
  FaceMarkerTest();

  void run() override;

private:
  void testSelection();
  void testGeometry();
  void testNormals();
  void testMasks();
  void testRelativePositions();
  void testInteriorFaceWithinPatch();
};
