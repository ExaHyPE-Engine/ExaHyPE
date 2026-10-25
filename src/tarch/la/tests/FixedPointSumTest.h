// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class FixedPointSumTest;
} // namespace tarch::la::tests

class tarch::la::tests::FixedPointSumTest final : public tarch::tests::TestCase {
public:
  FixedPointSumTest();
  FixedPointSumTest(const FixedPointSumTest& other)            = delete;
  FixedPointSumTest(FixedPointSumTest&& other)                 = delete;
  FixedPointSumTest& operator=(const FixedPointSumTest& other) = delete;
  FixedPointSumTest& operator=(FixedPointSumTest&& other)      = delete;
  ~FixedPointSumTest() override                                = default;

  void run() override;

private:
  void testEveryOrderOfTheTermsReturnsTheSameBits();
  void testNegatedTermsReturnTheNegatedSum();
  void testTermsOnTheGridReturnTheirExactSum();
  void testOneTermReturnsItsValue();
  void testQuotientsRoundToTheNearestDouble();
  void testDeviceSumsLikeTheHost();
};
