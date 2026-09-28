// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class ParticleSetTest;
} // namespace exahype2::particles::tests

/**
 * Checks that a particle set keeps the particles whose position a cell of the rank holds, that
 * a moved particle reaches the rank that holds its new position and that the device copy
 * carries the values to a kernel and back.
 */
class exahype2::particles::tests::ParticleSetTest final : public tarch::tests::TestCase {
public:
  ParticleSetTest();
  ParticleSetTest(const ParticleSetTest& other)            = delete;
  ParticleSetTest(ParticleSetTest&& other)                 = delete;
  ParticleSetTest& operator=(const ParticleSetTest& other) = delete;
  ParticleSetTest& operator=(ParticleSetTest&& other)      = delete;
  ~ParticleSetTest() override                              = default;

  void run() override;

private:
  void testInsertKeepsTheParticlesOfTheRank();
  void testAPositionOnAFaceBelongsToTheLowerCell();
  void testRelocateMovesAParticleToTheCellThatHoldsIt();
  void testInsertAndRelocateCountTheParticlesOfAllRanks();
  void testUploadAndDownloadCarryTheValues();
  void testAHostWriteReachesTheDeviceCopy();
  void testRelocateKeepsTheDeviceValuesWhileNoParticleLeaves();
  void testReleaseFreesEveryArray();
};
