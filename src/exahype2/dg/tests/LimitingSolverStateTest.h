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
  LimitingSolverStateTest(const LimitingSolverStateTest&)            = delete;
  LimitingSolverStateTest(LimitingSolverStateTest&&)                 = delete;
  LimitingSolverStateTest& operator=(const LimitingSolverStateTest&) = delete;
  LimitingSolverStateTest& operator=(LimitingSolverStateTest&&)      = delete;
  ~LimitingSolverStateTest() override                                = default;

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
