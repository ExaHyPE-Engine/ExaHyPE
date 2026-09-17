// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::dataview::tests {
  class CellMarkerTest;
} // namespace peano4::dataview::tests

class peano4::dataview::tests::CellMarkerTest final : public tarch::tests::TestCase {
public:
  CellMarkerTest();

  void run() override;

private:
  void testFlags();
  void testGeometry();
  void testIsContained();
  void testOverlaps();
  void testInvokingParentCellsCentre();
};
