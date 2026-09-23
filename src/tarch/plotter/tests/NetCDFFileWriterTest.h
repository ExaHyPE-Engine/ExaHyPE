// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class NetCDFFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::NetCDFFileWriterTest final : public tarch::tests::TestCase {
public:
  NetCDFFileWriterTest();
  ~NetCDFFileWriterTest() override = default;

  void run() override;

private:
  void test2DSimpleTriangle();
  void test2DQuadWithData();
  void test3DHexahedronWithData();
  void testMultipleSubdomains2DWithData();

  /**
   * The collective write completes when only the global master contributes data and
   * every other rank stays empty.
   */
  void testHeterogeneousRanks();

  /**
   * The collective write completes when the ranks hold different non-zero subdomain
   * counts, because the writer issues one collective write per variable.
   */
  void testUnevenSubdomainCounts();

  void testEmptyPlotter();

  /** The plotter throws when a data field does not match the geometry count. */
  void testInconsistentDataSizeThrows();

  /** The writer throws when a cell type it does not support, such as a line, is plotted. */
  void testUnsupportedCellType();

  /**
   * The writer rejects a data identifier that carries a character which would corrupt
   * a NetCDF name or an HDF5 path.
   */
  void testInvalidIdentifierThrows();
};
