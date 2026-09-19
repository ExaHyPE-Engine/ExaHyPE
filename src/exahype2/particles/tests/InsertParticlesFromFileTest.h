// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class InsertParticlesFromFileTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::InsertParticlesFromFileTest final : public tarch::tests::TestCase {
public:
  InsertParticlesFromFileTest();
  InsertParticlesFromFileTest(const InsertParticlesFromFileTest&)            = delete;
  InsertParticlesFromFileTest(InsertParticlesFromFileTest&&)                 = delete;
  InsertParticlesFromFileTest& operator=(const InsertParticlesFromFileTest&) = delete;
  InsertParticlesFromFileTest& operator=(InsertParticlesFromFileTest&&)      = delete;
  ~InsertParticlesFromFileTest() override                                    = default;

  void run() override;

private:
  void testFileReaderPositionsInsideTheCellBecomeParticles();
  void testStoreInitialPositionWritesThePositionIntoTheParticleData();
};
