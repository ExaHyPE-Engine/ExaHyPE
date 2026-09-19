// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class SolverTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the sweep states and the time step size of the ADER-DG solver.
 */
class exahype2::dg::ader::tests::SolverTest final : public tarch::tests::TestCase {
public:
  SolverTest();
  SolverTest(const SolverTest&)            = delete;
  SolverTest(SolverTest&&)                 = delete;
  SolverTest& operator=(const SolverTest&) = delete;
  SolverTest& operator=(SolverTest&&)      = delete;
  ~SolverTest() override                   = default;

  void run() override;

private:
  void testPredictionPrecedesTheTimeStep();
  void testInitialPredictionSweepKeepsTheTimeStep();
  void testHangingFacesInsertAPredictionSweep();
  void testPlotAndSuspensionResumeAtTheRightSweep();
  void testLimiterStatesMapOntoTheGalerkinStates();
  void testAdaptiveTimeStepFollowsTheScaledEigenvalue();
  void testAFixedSolverStepsWithAFractionOfTheSmallestMeshWidth();
  void testALimitedSolverWalksThroughATroubledTimeStep();
  void testSolversOfEqualParametersKeepSeparateStatistics();
  void testSweepPredicatesFollowTheMarkerAndTheSweep();
};
