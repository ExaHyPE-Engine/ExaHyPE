// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class CellErrorIntegralTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the error norms accumulateCellError computes from the cell state and an exact solution.
 */
class exahype2::dg::ader::tests::CellErrorIntegralTest final : public tarch::tests::TestCase {
public:
  CellErrorIntegralTest();
  CellErrorIntegralTest(const CellErrorIntegralTest&)            = delete;
  CellErrorIntegralTest(CellErrorIntegralTest&&)                 = delete;
  CellErrorIntegralTest& operator=(const CellErrorIntegralTest&) = delete;
  CellErrorIntegralTest& operator=(CellErrorIntegralTest&&)      = delete;
  ~CellErrorIntegralTest() override                              = default;

  void run() override;

private:
  /**
   * An exact solution that matches the cell state overwrites the three norms with zero.
   */
  void testAnExactSolutionLeavesNoError();

  /**
   * An offset that every node carries gives the offset times the cell volume, the squared offset times
   * the cell volume and the offset itself.
   */
  void testAConstantOffsetWeighsTheCellVolume();

  /**
   * An unknown outside the list of measured unknowns adds nothing to any of the three norms.
   */
  void testOnlyTheMeasuredUnknownsCount();

  /**
   * The maximum norm holds the largest node difference of the cell.
   */
  void testTheMaximumNormPicksTheLargestNode();

  /**
   * The exact solution sees every node of the cell once, the first axis running fastest, together with
   * the state of that node.
   */
  void testEveryNodeIsVisitedOnceInOrder();
};
