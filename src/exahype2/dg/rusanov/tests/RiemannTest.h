// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rusanov::tests {
  class RiemannTest;
}

class exahype2::dg::rusanov::tests::RiemannTest final : public tarch::tests::TestCase {
public:
  RiemannTest();

  ~RiemannTest() override = default;

  void run() override;

private:
  /** Checks that an equal state on both sides of the face reduces the Rusanov flux to the physical flux. */
  void testSolveRiemannProblemForEqualStates();

  /** Checks that the jump term of the Rusanov flux scales with the maximum eigenvalue and the state jump. */
  void testSolveRiemannProblemAddsJumpTerm();
};
