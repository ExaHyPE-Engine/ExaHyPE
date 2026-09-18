// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class LinearCombinationTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::LinearCombinationTest final : public tarch::tests::TestCase {
public:
  LinearCombinationTest();
  LinearCombinationTest(const LinearCombinationTest&)            = delete;
  LinearCombinationTest(LinearCombinationTest&&)                 = delete;
  LinearCombinationTest& operator=(const LinearCombinationTest&) = delete;
  LinearCombinationTest& operator=(LinearCombinationTest&&)      = delete;
  ~LinearCombinationTest() override                              = default;

  void run() override;

private:
  void testEachStepReadsItsOwnRowOfTheTableau();
  void testTheFinalStepCombinesInPlace();
  void testZeroWeightsSkipUncomputedEstimates();
};
