// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class VectorTest;
} // namespace tarch::la::tests

class tarch::la::tests::VectorTest final : public tarch::tests::TestCase {
public:
  VectorTest();
  VectorTest(const VectorTest& other)            = delete;
  VectorTest(VectorTest&& other)                 = delete;
  VectorTest& operator=(const VectorTest& other) = delete;
  VectorTest& operator=(VectorTest&& other)      = delete;
  ~VectorTest() override                         = default;

  void run() override;

private:
  void testConstruction();
  void testListConstructionIsAConstantExpression();
  void testDefaultConstructionZeroesEveryEntry();
  void testAssignment();
  void testVectorOperations();
  void testVectorScalarOperations();
  void testVectorVectorOperations();
  void testVectorCompare();
  void testVectorVectorCompare();
  void testSpecialisedVectorLayouts();
  void testAdditionalVectorOperations();
  void testSpecialisedVectorMembers();
  void testNaNComparesUnequal();
  void testSmallerComparisonsHonourTheTolerance();
  void testNorm2SquaredHoldsASwapOfTheFirstTwoComponents();
};
