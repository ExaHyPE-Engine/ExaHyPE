// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PointSourceContributionTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the point source kernels of the linear space-time predictor.
 */
class exahype2::dg::ader::tests::PointSourceContributionTest final : public tarch::tests::TestCase {
public:
  PointSourceContributionTest();
  PointSourceContributionTest(const PointSourceContributionTest&)            = delete;
  PointSourceContributionTest(PointSourceContributionTest&&)                 = delete;
  PointSourceContributionTest& operator=(const PointSourceContributionTest&) = delete;
  PointSourceContributionTest& operator=(PointSourceContributionTest&&)      = delete;
  ~PointSourceContributionTest() override                                    = default;

  void run() override;

private:
  /**
   * The basis function of a node returns one at that node and zero at every other node.
   */
  void testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers();

  /**
   * The basis functions weighted with the node coordinates return the position they are evaluated at.
   */
  void testTheBasisFunctionsReproduceALinearFunction();

  /**
   * An empty list of point sources leaves every sample at zero.
   */
  void testNoPointSourceLeavesTheSamplesEmpty();

  /**
   * A node receives the force of every listed source times the basis functions at the source position,
   * divided by the quadrature weight of the node and by the cell volume.
   */
  void testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume();

  /**
   * The zeroth Taylor coefficient receives the samples of the time nodes weighted with the quadrature
   * weights and the time step, and the sample at the start of the step stays out.
   */
  void testTheTimeIntegralSkipsTheSampleAtTheStart();

  /**
   * The derivative coefficients weight the samples into the unknowns of the state and leave the
   * auxiliary variables and the padding lanes alone.
   */
  void testTheDerivativeCoefficientsWeightTheUnknownsOnly();

  /**
   * Every axis of the time-averaged flux receives the same time integral divided by the number of axes.
   */
  void testEveryAxisReceivesTheSameTimeAverage();
};
