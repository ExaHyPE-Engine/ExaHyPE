// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class CauchyKovalewskiTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the steps of the Cauchy-Kovalewski recursion of the linear space-time predictor.
 */
class exahype2::dg::ader::tests::CauchyKovalewskiTest final : public tarch::tests::TestCase {
public:
  CauchyKovalewskiTest();
  CauchyKovalewskiTest(const CauchyKovalewskiTest&)            = delete;
  CauchyKovalewskiTest(CauchyKovalewskiTest&&)                 = delete;
  CauchyKovalewskiTest& operator=(const CauchyKovalewskiTest&) = delete;
  CauchyKovalewskiTest& operator=(CauchyKovalewskiTest&&)      = delete;
  ~CauchyKovalewskiTest() override                             = default;

  void run() override;

private:
  /**
   * The zeroth Taylor coefficient holds the cell state and a zero in every padding lane.
   */
  void testTheSeedCopiesTheCellStateAndClearsThePadding();

  /**
   * The time average starts at the zeroth Taylor coefficient.
   */
  void testTheTimeAverageStartsFromTheState();

  /**
   * A Taylor step starts from a zeroed flux, and a system without a source keeps that buffer.
   */
  void testTheTermsAreClearedBeforeATaylorStep();

  /**
   * A flux that is linear along its own axis differentiates to its constant physical slope.
   */
  void testALinearFluxDifferentiatesToItsPhysicalSlope();

  /**
   * A unit operator leaves the flux of every node in place, scaled by the extent of its axis.
   */
  void testTheUnitOperatorScalesEachAxisByItsCellExtent();

  /**
   * Every Taylor term enters the time average scaled by its factor, and a system without a source
   * leaves that buffer alone.
   */
  void testTheTimeAveragedTermsGrowByTheGivenFactor();

  /**
   * The unknowns of a Taylor coefficient enter the time-averaged state and its auxiliary variables
   * stay.
   */
  void testTheTimeAveragedStateGrowsByTheGivenFactor();

  /**
   * The next Taylor coefficient is the source minus the sum of the differentiated fluxes.
   */
  void testTheNextDerivativeIsTheSourceMinusTheFluxDivergence();
};
