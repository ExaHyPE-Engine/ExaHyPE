// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::particles::tests {
  class TestHelpers;
} // namespace peano4::particles::tests

class peano4::particles::tests::TestHelpers final : public tarch::tests::TestCase {
public:
  TestHelpers();

  void run() override;

private:
  /** Checks that a particle a neighbour rank sends stays local on the vertex that receives it. */
  void testParticleAssignedToVertexWillBeLocal();
};
