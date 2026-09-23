// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PredictTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the prediction body of one cell, its backup copy, its face projection and its labels.
 */
class exahype2::dg::ader::tests::PredictTest final : public tarch::tests::TestCase {
public:
  PredictTest();
  PredictTest(const PredictTest&)            = delete;
  PredictTest(PredictTest&&)                 = delete;
  PredictTest& operator=(const PredictTest&) = delete;
  PredictTest& operator=(PredictTest&&)      = delete;
  ~PredictTest() override                    = default;

  void run() override;

private:
  void testThePredictionCopiesTheCellStateIntoThePreviousBuffer();
  void testTheEstimatesReachTheCellsOwnFaceHalves();
  void testThePredictionStampsTheCellsOwnFaceHalves();
  void testTheCellReportsItselfToTheSolver();
  void testADivergedPredictorFlagsTheCell();
  void testThePredictorEvaluatesOnlyThePointSourcesInsideTheCell();
};
