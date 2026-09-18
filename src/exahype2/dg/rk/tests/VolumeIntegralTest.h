// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class VolumeIntegralTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::VolumeIntegralTest final : public tarch::tests::TestCase {
public:
  VolumeIntegralTest();
  VolumeIntegralTest(const VolumeIntegralTest&)            = delete;
  VolumeIntegralTest(VolumeIntegralTest&&)                 = delete;
  VolumeIntegralTest& operator=(const VolumeIntegralTest&) = delete;
  VolumeIntegralTest& operator=(VolumeIntegralTest&&)      = delete;
  ~VolumeIntegralTest() override                           = default;

  void run() override;

private:
  void testWeakFormOfAConstantFluxLiftsTheBasisEndpoints();
  void testLinearFormDifferentiatesTheFlux();
  void testSourceTermIsScaledByTheTimeStep();
  void testNonconservativeProductSeesTheNodalGradient();
  void testDiffusiveFluxSeesTheNodalGradients();
};
