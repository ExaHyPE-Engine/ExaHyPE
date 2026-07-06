// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class NetCDFFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::NetCDFFileWriterTest final : public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test2DQuadWithData();
  void test3DHexahedronWithData();
  void testMultipleSubdomains2DWithData();

  /**
   * Regression test for the collective field-schema deadlock: only the global
   * master contributes data while every other rank stays empty. The collective
   * parallel write must not deadlock, which relies on the field schema being
   * reduced globally so empty ranks join every variable call.
   */
  void testHeterogeneousRanks();

  /**
   * Ranks hold different non-zero subdomain counts (master 3, the rest 2) - the
   * real load-balanced distribution. The pre-fix writer issued one collective
   * nc_put_var* per local subdomain, so unequal counts mismatched the collective
   * call count across ranks and deadlocked; the merge-once writer issues one
   * collective write per variable. The old MPI tests only used uniform counts or
   * master-vs-empty, so they never caught this.
   */
  void testUnevenSubdomainCounts();

  void testEmptyPlotter();

  /**
   * The plotter must throw or fail safe if data does not match the geometry count.
   */
  void testInconsistentDataSizeThrows();

  /**
   * Checks that the writer throws if we try to plot unsupported cell types (e.g. lines).
   */
  void testUnsupportedCellType();

  /**
   * A data identifier with a character that would corrupt the NetCDF name / HDF5
   * path is rejected up front (shared FileWriter::validateIdentifier).
   */
  void testInvalidIdentifierThrows();

public:
  NetCDFFileWriterTest();
  ~NetCDFFileWriterTest() override = default;

  void run() override;
};
