// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class LoopTest;
} // namespace peano4::tests

class peano4::tests::LoopTest final : public tarch::tests::TestCase {
private:
  void testDForEachVisitsAllPointsInLinearOrder();
  void testDForEachWithPerAxisRange();
  void testDStartVector();
  void testDIncrementOverloadConsistency();
  void testDIncrementExclusive();
  void testDLinearised();
  void testDForEachExclusiveKeepsFixedDimensionAndIsComplete();
  void testDCompare();
  void testDCompareExclusive();
  void testDForEachCornerEnumeratesCornersInLinearisedOrder();
  void testDForEachStencilEnumeratesStencil();
  void testZForEach3IsAHamiltonianPath();
  void testGenericZForEachIsAHamiltonianPath();
  void testGenericZForEachMatchesZForEach3();

public:
  LoopTest();
  ~LoopTest() override = default;

  void run() override;
};
