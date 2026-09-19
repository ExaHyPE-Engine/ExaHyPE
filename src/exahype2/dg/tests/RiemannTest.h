// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class RiemannTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::RiemannTest final : public tarch::tests::TestCase {
public:
  RiemannTest();
  RiemannTest(const RiemannTest&)            = delete;
  RiemannTest(RiemannTest&&)                 = delete;
  RiemannTest& operator=(const RiemannTest&) = delete;
  RiemannTest& operator=(RiemannTest&&)      = delete;
  ~RiemannTest() override                    = default;

  void run() override;

private:
  void testEqualStatesAverageTheTwoFluxes();
  void testJumpTermScalesWithTheLargerWaveSpeed();
  void testBothSidesSeeTheSameFluxWithoutANonconservativeProduct();
  void testNonconservativeProductSplitsTheTwoSides();
  void testNonconservativeProductReadsTheAuxiliaryJump();
  void testLinearFormBuildsTheFluxFromTheStateJump();
  void testLinearFormAddsTheNonconservativeProduct();
  void testLinearFormAppliesTheMaterialParameterMatrix();
};
