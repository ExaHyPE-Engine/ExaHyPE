// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class StagesTest;
} // namespace exahype2::dg::rk::tests

/** Checks one Runge-Kutta stage of one cell on the host and on the device. */
class exahype2::dg::rk::tests::StagesTest final : public tarch::tests::TestCase {
public:
  StagesTest();
  StagesTest(const StagesTest&)            = delete;
  StagesTest(StagesTest&&)                 = delete;
  StagesTest& operator=(const StagesTest&) = delete;
  StagesTest& operator=(StagesTest&&)      = delete;
  ~StagesTest() override                   = default;

  void run() override;

private:
  void testTheCombinationWeighsTheEstimatesAndCopiesTheAuxiliaryVariables();
  void testAZeroWeightSkipsAnUnwrittenEstimate();
  void testTheCombinationWritesInPlace();
  void testAConstantFluxGivesZerosBitwise();
  void testTheDivergenceOfALinearFluxIsItsSlope();
  void testTheSourceTermIsScaledByTheTimeStep();
  void testTheNonconservativeProductSeesTheNodalGradient();
  void testTheDiffusiveFluxSeesTheNodalGradients();
  void testMirroredCellsReceiveMirroredEstimates();
  void testTheFaceStatesOfALinearStateAreItsValuesAtTheFaces();
  void testAConstantStateOpensAStageWithAZeroEstimateAndConstantFaceStates();
  void testTheFaceIntegralsReachTheEstimateOfTheStageAlone();
  void testTheCloseWritesTheWeightedSumInPlaceAndReportsTheWaveSpeeds();
  void testTheButcherTableausAreConsistent();
  void testEveryCellOfADeviceRangeOpensAStageLikeTheHost();
  void testEveryCellOfADeviceRangeOpensADiffusiveStageLikeTheHost();
  void testEveryCellOfADeviceRangeClosesTheStepLikeTheHost();

  template <typename Derived>
  void validateEveryCellOpensLikeTheHost();
};
