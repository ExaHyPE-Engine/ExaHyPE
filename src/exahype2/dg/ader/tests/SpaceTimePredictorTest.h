// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class SpaceTimePredictorTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the nonlinear and the linear space-time predictor of one cell against solutions they reproduce exactly.
 */
class exahype2::dg::ader::tests::SpaceTimePredictorTest final : public tarch::tests::TestCase {
public:
  SpaceTimePredictorTest();
  SpaceTimePredictorTest(const SpaceTimePredictorTest&)            = delete;
  SpaceTimePredictorTest(SpaceTimePredictorTest&&)                 = delete;
  SpaceTimePredictorTest& operator=(const SpaceTimePredictorTest&) = delete;
  SpaceTimePredictorTest& operator=(SpaceTimePredictorTest&&)      = delete;
  ~SpaceTimePredictorTest() override                               = default;

  void run() override;

private:
  /**
   * A nonlinear system without terms keeps the cell state, projects it onto every face and
   * converges in one sweep.
   */
  void testAnInertNonlinearSystemLeavesTheCellAtRest();

  /**
   * The nonlinear predictor moves a profile that is linear along the first axis by the amount its
   * flux, product, source and diffusive terms prescribe.
   */
  void testTheNonlinearPredictorMovesALinearProfileByTheExactAmount();

  /**
   * A flux that drives the iterate past the bound of the solve reports a diverged cell.
   */
  void testADivergingNonlinearPredictorReportsMinusOne();

  /**
   * A linear system without terms keeps the cell state, projects it onto every face and leaves the
   * flux of every face at zero.
   */
  void testAnInertLinearSystemLeavesTheCellAtRest();

  /**
   * The linear predictor translates a profile that is linear along the first axis by the flux and
   * the product times the time step size.
   */
  void testTheLinearPredictorTranslatesALinearProfileByTheExactAmount();

  /**
   * The Taylor recursion of a linear system with a flux, a product and a source reproduces the
   * time derivatives of that system.
   */
  void testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem();
};
