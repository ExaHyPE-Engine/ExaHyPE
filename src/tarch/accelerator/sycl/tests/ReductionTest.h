// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::sycl::tests {
  class ReductionTest;
} // namespace tarch::accelerator::sycl::tests

class tarch::accelerator::sycl::tests::ReductionTest final: public tarch::tests::TestCase {
private:
  void testParallelReduction();

public:
  ReductionTest();
  ~ReductionTest() override = default;

  void run() override;
};
