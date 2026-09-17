// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class FiniteVolumeTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::FiniteVolumeTest final : public tarch::tests::TestCase {
public:
  FiniteVolumeTest();

  void run() override;

private:
  void testZeroTimeStepPreservesState();
  void testUniformStateIsPreserved();
  void testLimitedLinearPreservesUniformState();
  void testLimitedLinearAverageConvergesAtSecondOrder();
  void testConstantConvergesAtFirstOrder();
  void testLimitedLinearMinmodKeepsStepInBounds();
  void testLimitedLinearZeroSlopesMatchConstant();
  void testLimitedLinearNcpAdvectionConvergesAtSecondOrder();
  void testPeriodicAdvectionConservesMass();
  void testSodShockTubeReachesStarStates();
  void testReconstructionHookReproducingDefaultChangesNothing();
  void testLimiterHookReproducingMinmodChangesNothing();
  void testParticleUpdateAdvancesWithExplicitEuler();
};
