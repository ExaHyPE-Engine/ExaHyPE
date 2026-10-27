// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class NonlinearPredictorTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the nonlinear space-time predictor of one cell on the host and on the device.
 */
class exahype2::dg::ader::tests::NonlinearPredictorTest final : public tarch::tests::TestCase {
public:
  NonlinearPredictorTest();
  NonlinearPredictorTest(const NonlinearPredictorTest& other)            = delete;
  NonlinearPredictorTest(NonlinearPredictorTest&& other)                 = delete;
  NonlinearPredictorTest& operator=(const NonlinearPredictorTest& other) = delete;
  NonlinearPredictorTest& operator=(NonlinearPredictorTest&& other)      = delete;
  ~NonlinearPredictorTest() override                                     = default;

  void run() override;

private:
  void testTheGuessRepeatsTheCellStateOverTime();
  void testTheRightHandSideOfATimeNodeStartsFromZero();
  void testAConstantFluxHasNoDivergence();
  void testALinearFluxSubtractsItsExactDivergence();
  void testAPicardSystemWithoutTermsReproducesTheInitialState();
  void testTheResidualCountsTheSquaredDistanceOfTheIterates();
  void testEveryThreadSplitSumsTheSameResidual();
  void testAnUnboundedIterateReportsDivergence();
  void testAnIterateAboveTheFloatRangeReportsDivergence();
  void testATinyToleranceComparesTheSquaredDistance();
  void testTheGradientOfAConstantVanishes();
  void testALinearStateHasTheExactGradient();
  void testTheFluxIsAskedForOncePerNodeAndAxis();
  void testTheDiffusiveFluxSeesTheNodalGradients();
  void testEveryTermCombinationAccumulatesIntoOneBuffer();
  void testTheAverageOfAConstantIsThatConstant();
  void testTheFirstTimeNodeRestartsTheRunningAverages();
  void testEveryAxisBlockKeepsCellNodeOrder();
  void testTheStateAverageLeavesTheLaterTimeSlicesAlone();
  void testTheScratchBlocksFollowEachOtherWithoutOverlap();
  void testATermTheSolverLacksHasZeroExtent();
  void testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState();
  void testAZeroSquaredToleranceReportsUnconvergedPicardIteration();
  void testTheToleranceGrowsToTheRoundingDistanceOfTheCell();
  void testAToleranceBelowTheRoundingDistanceKeepsTheTimeAverages();
  void testEveryCellOfADeviceRangeMatchesTheHostPredictor();

  template <typename Derived>
  void validateSourceOf(bool evaluateNonconservativeProduct, bool evaluateSource, bool evaluateDiffusiveSource);
};
