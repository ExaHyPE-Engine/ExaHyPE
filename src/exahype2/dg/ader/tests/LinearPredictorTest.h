// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class LinearPredictorTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the linear space-time predictor of one cell on the host and on the device.
 */
class exahype2::dg::ader::tests::LinearPredictorTest final : public tarch::tests::TestCase {
public:
  LinearPredictorTest();
  LinearPredictorTest(const LinearPredictorTest&)            = delete;
  LinearPredictorTest(LinearPredictorTest&&)                 = delete;
  LinearPredictorTest& operator=(const LinearPredictorTest&) = delete;
  LinearPredictorTest& operator=(LinearPredictorTest&&)      = delete;
  ~LinearPredictorTest() override                            = default;

  void run() override;

private:
  void testTheSeedCopiesTheCellStateAndClearsThePadding();
  void testTheTimeAverageStartsFromTheState();
  void testTheFluxIsClearedBeforeATaylorStep();
  void testTheLastPassEvaluatesTheTermsOfTheTimeAveragedState();
  void testTheTimeAveragedStateGrowsByTheGivenFactor();
  void testTheNextDerivativeIsTheSourceMinusTheFluxDivergence();
  void testTheFluxFillsEveryNodeAndAxis();
  void testAConstantStateLeavesTheNonconservativeProductAtZero();
  void testALinearStateGivesTheExactNonconservativeProduct();
  void testTheMaterialParameterMatrixActsOnEveryAxis();
  void testTheSourceOverwritesTheUnknownsOfEveryNode();
  void testASourceInsideIsSelected();
  void testASourceOutsideIsRejected();
  void testASourceOnAFaceBelongsToBothCells();
  void testSelectionKeepsTheTableOrder();
  void testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers();
  void testTheBasisFunctionsReproduceALinearFunction();
  void testNoPointSourceLeavesTheSamplesEmpty();
  void testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume();
  void testTheTimeIntegralSkipsTheSampleAtTheStart();
  void testTheDerivativeCoefficientsWeightTheUnknownsOnly();
  void testEveryAxisReceivesTheSameTimeAverage();
  void testTheScratchBlocksFollowEachOtherWithoutOverlap();
  void testATermTheSolverLacksHasZeroExtent();
  void testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState();
  void testASolverWithoutFluxGetsZeroFaceFluxes();
  void testEveryCellOfADeviceRangeSelectsItsPointSources();
  void testEveryCellOfADeviceRangeMatchesTheHostPredictor();
};
