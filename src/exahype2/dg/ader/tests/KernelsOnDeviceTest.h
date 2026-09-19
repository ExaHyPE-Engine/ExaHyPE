// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class KernelsOnDeviceTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks that both predictors, run over many cells through parallelFor on device memory,
 * reproduce the serial host loop; between them they reach every ADER kernel.
 */
class exahype2::dg::ader::tests::KernelsOnDeviceTest final : public tarch::tests::TestCase {
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
  void testSelectPointSources();
  void testNonlinearSpaceTimePredictor();
  void testLinearSpaceTimePredictor();

private:
  void validateBuffersMatch(
    const std::vector<double>& deviceResult,
    const std::vector<double>& hostResult,
    const std::string&         bufferName
  );
};
