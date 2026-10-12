// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::limiter::tests {
  class PolynomialScalingTest;
} // namespace exahype2::fv::limiter::tests

class exahype2::fv::limiter::tests::PolynomialScalingTest final : public tarch::tests::TestCase {
public:
  PolynomialScalingTest();
  PolynomialScalingTest(const PolynomialScalingTest&)            = delete;
  PolynomialScalingTest(PolynomialScalingTest&&)                 = delete;
  PolynomialScalingTest& operator=(const PolynomialScalingTest&) = delete;
  PolynomialScalingTest& operator=(PolynomialScalingTest&&)      = delete;
  ~PolynomialScalingTest() override                              = default;

  void run() override;

private:
  void testAdmissiblePolynomialIsUnchanged();
  void testFaceUndershootIsScaled();
  void testInadmissibleAverageGivesConstant();
};
