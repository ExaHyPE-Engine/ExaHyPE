// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class InsertParticlesByCoordinatesAndDataTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest final : public tarch::tests::TestCase {
public:
  InsertParticlesByCoordinatesAndDataTest();
  InsertParticlesByCoordinatesAndDataTest(const InsertParticlesByCoordinatesAndDataTest&)            = delete;
  InsertParticlesByCoordinatesAndDataTest(InsertParticlesByCoordinatesAndDataTest&&)                 = delete;
  InsertParticlesByCoordinatesAndDataTest& operator=(const InsertParticlesByCoordinatesAndDataTest&) = delete;
  InsertParticlesByCoordinatesAndDataTest& operator=(InsertParticlesByCoordinatesAndDataTest&&)      = delete;
  ~InsertParticlesByCoordinatesAndDataTest() override                                                = default;

  void run() override;

private:
  void testEveryCoordinateOfTheDataArrayEntersOneCellOnce();
  void testCoordinateOnALowerFaceEntersTheCellAboveTheDomainWallOnly();
};
