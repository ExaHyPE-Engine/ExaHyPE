// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::kokkos::tests {
  class ReductionTest;
} // namespace tarch::accelerator::kokkos::tests

class tarch::accelerator::kokkos::tests::ReductionTest final : public tarch::tests::TestCase {
public:
  ReductionTest();
  ~ReductionTest() override = default;

  void run() override;

private:
  void testParallelReduction();
};
