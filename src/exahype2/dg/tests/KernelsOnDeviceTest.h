// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class KernelsOnDeviceTest;
} // namespace exahype2::dg::tests

/**
 * Checks that every DG kernel run over many cells or faces through
 * tarch::accelerator::parallelFor over device memory reproduces the serial host loop.
 */
class exahype2::dg::tests::KernelsOnDeviceTest final : public tarch::tests::TestCase {
public:
  KernelsOnDeviceTest();
  KernelsOnDeviceTest(const KernelsOnDeviceTest&)            = delete;
  KernelsOnDeviceTest(KernelsOnDeviceTest&&)                 = delete;
  KernelsOnDeviceTest& operator=(const KernelsOnDeviceTest&) = delete;
  KernelsOnDeviceTest& operator=(KernelsOnDeviceTest&&)      = delete;
  ~KernelsOnDeviceTest() override                            = default;

  void run() override;

  // These test bodies launch extended __device__ lambdas, which nvcc forbids
  // inside private or protected member functions.
  void testExtrapolateStateToFaces();
  void testExtrapolateFluxToFaces();
  void testEvaluateFluxOnFace();
  void testApplyBoundaryConditions();
  void testSolveRiemannProblem();
  void testSolveLinearRiemannProblem();
  void testAddFaceIntegral();
  void testReduceMaxEigenvalues();

private:
  void validateBuffersMatch(
    const std::vector<double>& deviceResult,
    const std::vector<double>& hostResult,
    const std::string&         bufferName
  );
};
