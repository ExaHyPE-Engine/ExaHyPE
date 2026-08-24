// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class ExactAccumulatorTest;
} // namespace tarch::la::tests

class tarch::la::tests::ExactAccumulatorTest final : public tarch::tests::TestCase {
private:
  void testCorrectRounding();
  void testOrderInvariance();
  void testNegationSymmetry();
  void testSubnormalsAndZero();
  void testClear();

public:
  ExactAccumulatorTest();
  ~ExactAccumulatorTest() override = default;
  void run() override;
};
