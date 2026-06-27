// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::kokkos::tests {
  class ReductionTest;
} // namespace tarch::accelerator::kokkos::tests

class tarch::accelerator::kokkos::tests::ReductionTest final : public tarch::tests::TestCase {
public:
  // The test bodies launch backend kernels via extended __device__ lambdas,
  // which nvcc forbids inside private/protected member functions, so these are
  // public.
  void testParallelReduction();

public:
  ReductionTest();
  ~ReductionTest() override = default;

  void run() override;
};
