// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class LoopTest;
} // namespace peano4::tests

class peano4::tests::LoopTest final : public tarch::tests::TestCase {
public:
  LoopTest();
  ~LoopTest() override = default;

  void run() override;

private:
  void testDForEachVisitsAllPointsInLinearOrder();
  void testDForEachWithPerAxisRange();
  void testDLinearised();
  void testDDelinearisedInvertsDLinearised();
  void testDDelinearisedExclusiveKeepsFixedDimensionAtZero();
  void testDLinearisedExclusiveInvertsDDelinearisedExclusive();
  void testDForEachCornerEnumeratesCornersInLinearisedOrder();
};
