// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class FiniteVolumeSchemeTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::FiniteVolumeSchemeTest final : public tarch::tests::TestCase {
public:
  FiniteVolumeSchemeTest();
  FiniteVolumeSchemeTest(const FiniteVolumeSchemeTest& other)            = delete;
  FiniteVolumeSchemeTest(FiniteVolumeSchemeTest&& other)                 = delete;
  FiniteVolumeSchemeTest& operator=(const FiniteVolumeSchemeTest& other) = delete;
  FiniteVolumeSchemeTest& operator=(FiniteVolumeSchemeTest&& other)      = delete;
  ~FiniteVolumeSchemeTest() override                                     = default;

  void run() override;

private:
  void testTheFaceVolumeIndexRunsLayerSlowest();
  void testBothFaceVolumeIndexFormsAgreeAndVisitEveryVolumeOnce();
  void testForEachVolumeVisitsEveryVolumeInLinearisedOrder();
  void testAccumulateMaxEigenvalueKeepsTheLargestAndAFault();
  void testInitialiseWritesTheInitialConditionAtEveryVolumeCentre();
  void testBoundaryConditionsCopyAUniformStateOntoTheOuterHalf();
  void testEachOuterLayerReadsTheMatchingInnerLayer();
  void testEachOuterVolumeIsWrittenOnce();
  void testProjectionFillsTheOwnHalfOfEveryFace();
  void testHaloInterpolationReproducesALinearProfile();
  void testAConstantFaceInterpolatesBitwise();
  void testHaloInjectionCopiesTheCoveringVolume();
  void testHaloRestrictionAveragesTheCoveringVolumes();
  void testConstantFineFacesRestrictBitwise();
  void testPatchInterpolationCopiesTheCoveringVolume();
  void testPatchRestrictionInvertsTheInterpolation();
  void testPatchRestrictionAveragesExactly();
  void testAVolumeFaceOnTheDomainBoundaryReachesTheRiemannSolver();
  void testZeroTimeStepPreservesState();
  void testUniformStateIsPreserved();
  void testAFixedRuleReportsNoWaveSpeed();
  void testASourceTermGrowsAUniformStateByItsRate();
  void testADiffusiveSourceTermReadsTheForwardDifference();
  void testLimitedLinearPreservesUniformState();
  void testLimitedLinearIntegratesTimeDependentAdvection();
  void testLimitedLinearAverageConvergesAtSecondOrder();
  void testConstantConvergesAtFirstOrder();
  void testLimitedLinearMinmodKeepsStepInBounds();
  void testLimitedLinearZeroSlopesMatchConstant();
  void testLimitedLinearNcpAdvectionConvergesAtSecondOrder();
  void testLimitedLinearCustomRiemannIncludesVolumeNonconservativeProduct();
  void testPeriodicAdvectionConservesMass();
  void testSodShockTubeReachesStarStates();
  void testMirroredVolumesReceiveMirroredStates();
  void testReconstructionHookReproducingDefaultChangesNothing();
  void testLimiterHookReproducingMinmodChangesNothing();
  void testThePostprocessingRunsBeforeTheWaveSpeedsOfTheUpdatedPatch();
  void testReduceMaxEigenvalueTakesTheLargestWaveSpeedOverTheVolumes();
  void testMeasureErrorsSumsTheWeightedNorms();
  void testEveryCellOfADeviceRangeUpdatesLikeTheHost();
  void testTheTermRingKeepsAWindowOfEntries();
  void testTheSweepUpdatesLikeTheVolumeUpdates();
  void testTheSweepUpdatesInPlaceLikeIntoASecondPatch();
};
