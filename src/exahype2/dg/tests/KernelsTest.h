// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class KernelsTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::KernelsTest final : public tarch::tests::TestCase {
public:
  KernelsTest();
  KernelsTest(const KernelsTest&)            = delete;
  KernelsTest(KernelsTest&&)                 = delete;
  KernelsTest& operator=(const KernelsTest&) = delete;
  KernelsTest& operator=(KernelsTest&&)      = delete;
  ~KernelsTest() override                    = default;

  void run() override;

private:
  void testFaceBuffersAreClearedBeforeTheProjection();
  void testAConstantStateReachesEveryFace();
  void testEachFaceNodeSeesOnlyItsOwnLine();
  void testTheTwoEndsUseTheirOwnCoefficients();
  void testAStateOfFloatReachesFacesOfDouble();
  void testFluxIsEvaluatedAtCellNodesBeforeExtrapolation();
  void testASystemWithoutFluxLeavesZeroesBehind();
  void testDiffusiveFluxIsExtrapolatedWithTheNodalGradients();
  void testTheOuterHalfIsFilledFromTheInnerHalf();
  void testEveryNodeSeesItsOwnPositionOnTheFace();
  void testFluxOnAFaceComesFromTheFaceState();
  void testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal();
  void testEqualStatesLeaveHalfTheFluxJumpOnBothSides();
  void testEqualSidesLeaveZerosBitwise();
  void testJumpTermScalesWithTheLargerWaveSpeed();
  void testTheTwoDifferencesAddUpToTheFluxJump();
  void testNonconservativeProductEntersBothSidesAlike();
  void testNonconservativeProductReadsTheAuxiliaryJump();
  void testLinearFormBuildsTheFluxFromTheStateJump();
  void testLinearFormAddsTheNonconservativeProduct();
  void testLinearFormAppliesTheMaterialParameterMatrix();
  void testAnInteriorFaceHoldsTheDifferencesToItsOwnFluxes();
  void testABoundaryFaceTakesTheBoundaryState();
  void testTheSolversRiemannSolverReceivesTheFace();
  void testTheLinearRiemannSolverWritesOppositeDissipations();
  void testZeroFluxLeavesUpdateUntouched();
  void testEachCellNodeReceivesItsOwnFaceNode();
  void testUpdateStridesByTheUnknowns();
  void testBothOrientationsEnterWithTheSameSign();
  void testWeightsAndCoefficientsEnterAsAProduct();
  void testANodeReceivesOnlyItsOwnUpdate();
  void testAnisotropicAccumulation();
  void testInvalidEigenvalueAccumulation();
  void testReduceMaxEigenvaluesOverTheCellNodes();
  void testTheReductionKeepsTheLargerAndAnInvalidObservation();
  void testCopyKeepsEveryValue();
  void testInterpolationFromTheParentReproducesALinearState();
  void testRestrictionFromTheChildrenReproducesALinearState();
  void testExtrapolateStateToFacesOnTheDevice();
  void testExtrapolateFluxToFacesOnTheDevice();
  void testEvaluateFluxOnFaceOnTheDevice();
  void testApplyBoundaryConditionsOnTheDevice();
  void testSolveRiemannProblemOnTheDevice();
  void testSolveLinearRiemannProblemOnTheDevice();
  void testAddFaceIntegralOnTheDevice();
  void testReduceMaxEigenvaluesOnTheDevice();
};
