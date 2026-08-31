// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class ExactAccumulatorTest;
} // namespace tarch::la::tests

class tarch::la::tests::ExactAccumulatorTest final : public tarch::tests::TestCase {
public:
  ExactAccumulatorTest();
  ExactAccumulatorTest(const ExactAccumulatorTest&)            = delete;
  ExactAccumulatorTest(ExactAccumulatorTest&&)                 = delete;
  ExactAccumulatorTest& operator=(const ExactAccumulatorTest&) = delete;
  ExactAccumulatorTest& operator=(ExactAccumulatorTest&&)      = delete;
  ~ExactAccumulatorTest() override                             = default;

  void run() override;

private:
  void testCorrectRounding();
  void testOrderInvariance();
  void testNegationSymmetry();
  void testSubnormalsAndZero();
  void testOverflow();
  void testClear();
};
