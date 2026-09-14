// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace accelerator {
    namespace tests {
      class ParallelConstructsTest;
    } // namespace tests
  } // namespace accelerator
} // namespace tarch

class tarch::accelerator::tests::ParallelConstructsTest final : public tarch::tests::TestCase {
public:
  // The test bodies launch backend kernels via extended __device__ lambdas,
  // which nvcc forbids inside private/protected member functions, so these are
  // public.
  void testParallelForOnHost();
  void testParallelFor();
  void testParallelForRangeMD2();
  void testParallelForRangeMD3();
  void testParallelForTeam();
  void testParallelReduceSum();
  void testParallelReduceMin();
  void testParallelReduceMax();
  void testParallelReduceRangeMD2();
  void testParallelReduceRangeMD3();
  void testParallelReduceTeam();

  ParallelConstructsTest();
  ~ParallelConstructsTest() override = default;

  void run() override;
};
