// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PredictorTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the predictor phase of one cell against solutions it reproduces exactly.
 */
class exahype2::dg::ader::tests::PredictorTest final : public tarch::tests::TestCase {
public:
  PredictorTest();
  PredictorTest(const PredictorTest&)            = delete;
  PredictorTest(PredictorTest&&)                 = delete;
  PredictorTest& operator=(const PredictorTest&) = delete;
  PredictorTest& operator=(PredictorTest&&)      = delete;
  ~PredictorTest() override                      = default;

  void run() override;

private:
  void testAnInertNonlinearSystemLeavesTheCellAtRest();
  void testTheNonlinearPredictorMovesALinearProfileByTheExactAmount();
  void testADivergingNonlinearPredictorReportsDivergence();
  void testThePredictionCountsAddUpOverCells();
  void testAnInertLinearSystemLeavesTheCellAtRest();
  void testTheLinearPredictorTranslatesALinearProfileByTheExactAmount();
  void testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem();
  void testTheVolumeIntegralSubtractsTheDivergenceAndAddsTheSource();

  void validateFaceStates(const double* const* faceStates, const double* expectedState);
  void validateFaceFluxes(const double* const* faceFluxes, const double* expectedFlux);
};
