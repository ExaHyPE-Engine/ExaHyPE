// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class VolumeIntegralTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the volume integral of the ADER-DG predictor against the weak form it discretises.
 */
class exahype2::dg::ader::tests::VolumeIntegralTest final : public tarch::tests::TestCase {
public:
  VolumeIntegralTest();
  VolumeIntegralTest(const VolumeIntegralTest&)            = delete;
  VolumeIntegralTest(VolumeIntegralTest&&)                 = delete;
  VolumeIntegralTest& operator=(const VolumeIntegralTest&) = delete;
  VolumeIntegralTest& operator=(VolumeIntegralTest&&)      = delete;
  ~VolumeIntegralTest() override                           = default;

  void run() override;

private:
  /**
   * A coefficient holds the time step size times the stiffness entry over the weight of its row.
   */
  void testTheCoefficientsCarryTheTimeStepAndTheRowWeight();

  /**
   * A constant flux integrates to the difference of the basis function at the two ends of every
   * axis.
   */
  void testAConstantFluxLiftsTheBasisEndpoints();

  /**
   * The time-averaged source enters the cell state scaled by the time step size.
   */
  void testTheSourceScalesWithTheTimeStep();

  /**
   * The integral of a linear system subtracts the sum over the axes and adds the source.
   */
  void testTheLinearIntegralSubtractsTheSumOverTheAxes();
};
