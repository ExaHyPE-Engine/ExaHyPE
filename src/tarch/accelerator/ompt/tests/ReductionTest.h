// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::ompt::tests {
  class ReductionTest;
} // namespace tarch::accelerator::ompt::tests

class tarch::accelerator::ompt::tests::ReductionTest final: public tarch::tests::TestCase {
private:
  void testParallelReduction();

public:
  ReductionTest();
  ~ReductionTest() override = default;

  void run() override;
};
