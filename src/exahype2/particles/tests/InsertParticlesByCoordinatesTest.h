// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class InsertParticlesByCoordinatesTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::InsertParticlesByCoordinatesTest final : public tarch::tests::TestCase {
public:
  InsertParticlesByCoordinatesTest();
  InsertParticlesByCoordinatesTest(const InsertParticlesByCoordinatesTest&)            = delete;
  InsertParticlesByCoordinatesTest(InsertParticlesByCoordinatesTest&&)                 = delete;
  InsertParticlesByCoordinatesTest& operator=(const InsertParticlesByCoordinatesTest&) = delete;
  InsertParticlesByCoordinatesTest& operator=(InsertParticlesByCoordinatesTest&&)      = delete;
  ~InsertParticlesByCoordinatesTest() override                                         = default;

  void run() override;

private:
  void testInsertsOneParticlePerOwnedCoordinate();
  void testRefinedCellInsertsNoParticle();
};
