// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class InteractionSetsTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::InteractionSetsTest final : public tarch::tests::TestCase {
public:
  InteractionSetsTest();
  InteractionSetsTest(const InteractionSetsTest&)            = delete;
  InteractionSetsTest(InteractionSetsTest&&)                 = delete;
  InteractionSetsTest& operator=(const InteractionSetsTest&) = delete;
  InteractionSetsTest& operator=(InteractionSetsTest&&)      = delete;
  ~InteractionSetsTest() override                            = default;

  void run() override;

private:
  void testCollectParticlesOfCell();
  void testUpdateContainedInAdjacentCellOfLocalParticles();
  void testReleaseParticlesOfCell();
};
