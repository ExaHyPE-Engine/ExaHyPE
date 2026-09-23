// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PointSourcesTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks which point sources of the solver table selectPointSources writes into the index list of a cell.
 */
class exahype2::dg::ader::tests::PointSourcesTest final : public tarch::tests::TestCase {
public:
  PointSourcesTest();
  PointSourcesTest(const PointSourcesTest&)            = delete;
  PointSourcesTest(PointSourcesTest&&)                 = delete;
  PointSourcesTest& operator=(const PointSourcesTest&) = delete;
  PointSourcesTest& operator=(PointSourcesTest&&)      = delete;
  ~PointSourcesTest() override                         = default;

  void run() override;

private:
  /**
   * A point source inside the cell raises the count by one and writes its table index.
   */
  void testASourceInsideIsSelected();

  /**
   * A point source that leaves the cell along one axis raises neither the count nor an index.
   */
  void testASourceOutsideIsRejected();

  /**
   * A point source on a face of two cells appears in the list of both cells.
   */
  void testASourceOnAFaceBelongsToBothCells();

  /**
   * The indices follow the order of the point source table of the solver.
   */
  void testSelectionKeepsTheTableOrder();
};
