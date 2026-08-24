// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class LimitingSolverStateTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::LimitingSolverStateTest final : public tarch::tests::TestCase {
public:
  LimitingSolverStateTest();

  void run() override;

private:
  void testSmoothStepCostsTwoSweeps();
  void testHangingFacesInsertAPredictionSweep();
  void testTroubledStepSpreadsThenRecomputes();
  void testDomainGrowsExactlyTheNumberOfSpreadRings();
  void testOnlyThePredictorOpensAStep();
  void testStepClosesAtCorrectorOrRecompute();
  void testGridConstructionLeadsToTheFirstPrediction();
};
