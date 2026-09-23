// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class GradientTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::GradientTest final : public tarch::tests::TestCase {
public:
  GradientTest();
  GradientTest(const GradientTest&)            = delete;
  GradientTest(GradientTest&&)                 = delete;
  GradientTest& operator=(const GradientTest&) = delete;
  GradientTest& operator=(GradientTest&&)      = delete;
  ~GradientTest() override                     = default;

  void run() override;

private:
  void testGradientOfALinearPolynomialIsItsSlope();
  void testEachBlockHoldsTheDerivativeAlongItsOwnAxis();
  void testNarrowStorageIsDifferentiatedInTheComputePrecision();
};
