// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class SolverTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::SolverTest final : public tarch::tests::TestCase {
public:
  SolverTest();
  SolverTest(const SolverTest&)            = delete;
  SolverTest(SolverTest&&)                 = delete;
  SolverTest& operator=(const SolverTest&) = delete;
  SolverTest& operator=(SolverTest&&)      = delete;
  ~SolverTest() override                   = default;

  void run() override;

private:
  void testStagesAlternateAfterInitialisation();
  void testMeshRefinementSweepPrecedesTheStage();
  void testPlotAfterInitialisationReturnsToTheFirstTimeStep();
  void testSuspendedSweepSchedulesTheFirstStage();
  void testAdaptiveTimeStepFollowsTheDampedEigenvalue();
  void testFixedTimeStepFollowsTheSmallestCell();
  void testSolversOfEqualParametersKeepSeparateStatistics();
  void testSweepPredicatesFollowTheMarkerAndTheSweep();
};
