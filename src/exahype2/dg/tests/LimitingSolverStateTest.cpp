// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimitingSolverStateTest.h"

#include "exahype2/dg/LimitingSolverState.h"

exahype2::dg::tests::LimitingSolverStateTest::LimitingSolverStateTest() :
  tarch::tests::TestCase("exahype2::dg::tests::LimitingSolverStateTest") {}

void exahype2::dg::tests::LimitingSolverStateTest::run() {
  testMethod(testSmoothStepCostsTwoSweeps);
  testMethod(testHangingFacesInsertAPredictionSweep);
  testMethod(testTroubledStepSpreadsThenRecomputes);
  testMethod(testDomainGrowsExactlyTheNumberOfSpreadRings);
  testMethod(testOnlyThePredictorOpensAStep);
  testMethod(testStepClosesAtCorrectorOrRecompute);
  testMethod(testGridConstructionLeadsToTheFirstPrediction);
}

void exahype2::dg::tests::LimitingSolverStateTest::testSmoothStepCostsTwoSweeps() {
  constexpr int                              NumberOfSpreadRings = 2;
  const exahype2::dg::LimitingBranchDecision smooth{false, false};
  exahype2::dg::LimitingSweep                sweep{exahype2::dg::LimitingSolverState::Predict, 0};

  sweep = exahype2::dg::advanceLimitingSweep(sweep, smooth, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::CorrectAndFlag));

  sweep = exahype2::dg::advanceLimitingSweep(sweep, smooth, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::Predict));
}

void exahype2::dg::tests::LimitingSolverStateTest::testHangingFacesInsertAPredictionSweep() {
  constexpr int                              NumberOfSpreadRings = 2;
  const exahype2::dg::LimitingBranchDecision hanging{true, false};
  exahype2::dg::LimitingSweep                sweep{exahype2::dg::LimitingSolverState::Predict, 0};

  sweep = exahype2::dg::advanceLimitingSweep(sweep, hanging, NumberOfSpreadRings);
  validateEquals(
    static_cast<int>(sweep.state),
    static_cast<int>(exahype2::dg::LimitingSolverState::PredictOnHangingCells)
  );

  sweep = exahype2::dg::advanceLimitingSweep(sweep, hanging, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::CorrectAndFlag));
}

void exahype2::dg::tests::LimitingSolverStateTest::testTroubledStepSpreadsThenRecomputes() {
  constexpr int                              NumberOfSpreadRings = 2;
  const exahype2::dg::LimitingBranchDecision troubled{false, true};
  exahype2::dg::LimitingSweep                sweep{exahype2::dg::LimitingSolverState::CorrectAndFlag, 0};

  sweep = exahype2::dg::advanceLimitingSweep(sweep, troubled, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::SpreadDomain));
  validateEquals(sweep.spreadRing, 0);

  sweep = exahype2::dg::advanceLimitingSweep(sweep, troubled, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::SpreadDomain));
  validateEquals(sweep.spreadRing, 1);

  sweep = exahype2::dg::advanceLimitingSweep(sweep, troubled, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::RecomputeSubcells));

  sweep = exahype2::dg::advanceLimitingSweep(sweep, troubled, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::Predict));
}

void exahype2::dg::tests::LimitingSolverStateTest::testDomainGrowsExactlyTheNumberOfSpreadRings() {
  const exahype2::dg::LimitingBranchDecision troubled{false, true};
  for (int numberOfSpreadRings = 1; numberOfSpreadRings <= 4; numberOfSpreadRings++) {
    exahype2::dg::LimitingSweep sweep{exahype2::dg::LimitingSolverState::CorrectAndFlag, 0};
    int                         spreadSweeps = 0;
    sweep                                    = exahype2::dg::advanceLimitingSweep(sweep, troubled, numberOfSpreadRings);
    while (sweep.state == exahype2::dg::LimitingSolverState::SpreadDomain) {
      validateEquals(sweep.spreadRing, spreadSweeps);
      spreadSweeps++;
      sweep = exahype2::dg::advanceLimitingSweep(sweep, troubled, numberOfSpreadRings);
    }
    validateEquals(spreadSweeps, numberOfSpreadRings);
    validateEquals(
      static_cast<int>(sweep.state),
      static_cast<int>(exahype2::dg::LimitingSolverState::RecomputeSubcells)
    );
  }
}

void exahype2::dg::tests::LimitingSolverStateTest::testOnlyThePredictorOpensAStep() {
  validate(exahype2::dg::isFirstSweepOfTimeStep({exahype2::dg::LimitingSolverState::Predict, 0}));
  validate(not exahype2::dg::isFirstSweepOfTimeStep({exahype2::dg::LimitingSolverState::PredictOnHangingCells, 0}));
  validate(not exahype2::dg::isFirstSweepOfTimeStep({exahype2::dg::LimitingSolverState::CorrectAndFlag, 0}));
  validate(not exahype2::dg::isFirstSweepOfTimeStep({exahype2::dg::LimitingSolverState::SpreadDomain, 0}));
  validate(not exahype2::dg::isFirstSweepOfTimeStep({exahype2::dg::LimitingSolverState::RecomputeSubcells, 0}));
}

void exahype2::dg::tests::LimitingSolverStateTest::testStepClosesAtCorrectorOrRecompute() {
  const exahype2::dg::LimitingBranchDecision smooth{false, false};
  const exahype2::dg::LimitingBranchDecision troubled{false, true};

  validate(exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::CorrectAndFlag, 0}, smooth));
  validate(exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::RecomputeSubcells, 0}, troubled));

  validate(not exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::CorrectAndFlag, 0}, troubled));
  validate(not exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::Predict, 0}, smooth));
  validate(not exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::SpreadDomain, 0}, troubled));
  validate(
    not exahype2::dg::isLastSweepOfTimeStep({exahype2::dg::LimitingSolverState::PredictOnHangingCells, 0}, smooth)
  );
}

void exahype2::dg::tests::LimitingSolverStateTest::testGridConstructionLeadsToTheFirstPrediction() {
  constexpr int                              NumberOfSpreadRings = 2;
  const exahype2::dg::LimitingBranchDecision smooth{false, false};

  exahype2::dg::LimitingSweep sweep{exahype2::dg::LimitingSolverState::GridConstruction, 0};
  sweep = exahype2::dg::advanceLimitingSweep(sweep, smooth, NumberOfSpreadRings);
  validateEquals(
    static_cast<int>(sweep.state),
    static_cast<int>(exahype2::dg::LimitingSolverState::GridInitialisation)
  );

  sweep = exahype2::dg::advanceLimitingSweep(sweep, smooth, NumberOfSpreadRings);
  validateEquals(static_cast<int>(sweep.state), static_cast<int>(exahype2::dg::LimitingSolverState::Predict));
}
