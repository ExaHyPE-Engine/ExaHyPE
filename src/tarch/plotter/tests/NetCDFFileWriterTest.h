// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class NetCDFFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::NetCDFFileWriterTest final: public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test2DQuadWithData();
  void testEmptyPlotter();

  /**
   * Verifies that the plotter behaves correctly (throws or fails safe)
   * if we define data that doesn't match the geometry count.
   */
  void testInconsistentDataSizeThrows();

  /**
   * Checks that the writer throws if we try to plot unsupported cell types (e.g. Lines or Hexahedrons),
   * since the NetCDF writer is specialised for UGRID 2D (Triangles/Quads).
   */
  void testUnsupportedCellType();

public:
  NetCDFFileWriterTest();
  ~NetCDFFileWriterTest() override = default;

  void run() override;
};
