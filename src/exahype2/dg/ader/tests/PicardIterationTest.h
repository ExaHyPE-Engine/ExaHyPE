// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PicardIterationTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the kernels of one Picard sweep of the nonlinear space-time predictor.
 */
class exahype2::dg::ader::tests::PicardIterationTest final : public tarch::tests::TestCase {
public:
  PicardIterationTest();
  PicardIterationTest(const PicardIterationTest&)            = delete;
  PicardIterationTest(PicardIterationTest&&)                 = delete;
  PicardIterationTest& operator=(const PicardIterationTest&) = delete;
  PicardIterationTest& operator=(PicardIterationTest&&)      = delete;
  ~PicardIterationTest() override                            = default;

  void run() override;

private:
  /**
   * Every time slice of the space-time state holds the cell state and every padding lane holds zero.
   */
  void testTheGuessRepeatsTheCellStateOverTime();

  /**
   * The right-hand side of a time node holds the cell state times the cell weight and the temporal basis function.
   */
  void testTheRightHandSideCarriesTheInitialValue();

  /**
   * A flux that is constant along every axis leaves the right-hand side where it is.
   */
  void testAConstantFluxHasNoDivergence();

  /**
   * A flux that grows linearly along its own axis subtracts its exact derivative from one time slice.
   */
  void testALinearFluxSubtractsItsExactDivergence();

  /**
   * A cell without flux and without source solves to the initial state at every time node.
   */
  void testAPicardSystemWithoutTermsReproducesTheInitialState();

  /**
   * The solve returns the squared distance between the new and the previous iterate and stores the new one.
   */
  void testTheResidualIsTheSquaredDistanceOfTheIterates();

  /**
   * An iterate above the divergence bound and an iterate that is not a number both return an infinite distance.
   */
  void testAnUnboundedIterateReportsDivergence();
};
