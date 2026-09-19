// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class UpdateStageTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::UpdateStageTest final : public tarch::tests::TestCase {
public:
  UpdateStageTest();
  UpdateStageTest(const UpdateStageTest&)            = delete;
  UpdateStageTest(UpdateStageTest&&)                 = delete;
  UpdateStageTest& operator=(const UpdateStageTest&) = delete;
  UpdateStageTest& operator=(UpdateStageTest&&)      = delete;
  ~UpdateStageTest() override                        = default;

  void run() override;

private:
  void testSweepAfterGridInitialisationOpensTheFirstStage();
  void testStageAddsTheFaceIntegralToThePrecedingEstimate();
  void testFirstSweepCombinesTheEstimatesAndStampsTheCell();
  void testSweepAfterGridInitialisationReportsTheEigenvalueOfTheCell();
  void testFirstSweepOfAnAdaptiveSolverTakesTheAdmissibleTimeStepSize();
  void testBoundaryFaceFillsTheExteriorTraceAndMirrorsTheTimeStamps();
  void testUpperBoundaryFaceCopiesTheTimeStampsOntoTheExteriorHalf();
  void testBoundaryFaceOfARefinedCellKeepsItsTraceAndFlux();
  void testPeriodicFaceKeepsBothTracesAndTimeStamps();
  void testInteriorFaceKeepsTheTimeStampsAndSolvesTheRiemannProblem();
  void testUserRiemannSolverReceivesTheFaceAndWritesBothFluxes();
  void testLinearFaceFluxesFollowTheStateJump();
  void testTimeStampsRollOverOnTheFirstSweepOnly();
};
