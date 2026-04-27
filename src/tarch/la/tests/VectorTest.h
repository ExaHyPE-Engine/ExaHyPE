// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class VectorTest;
} // namespace tarch::la::tests

class tarch::la::tests::VectorTest final: public tarch::tests::TestCase {
private:
  void testConstruction();
  void testAssignment();
  void testVectorOperations();
  void testVectorScalarOperations();
  void testVectorVectorOperations();
  void testVectorCompare();
  void testVectorVectorCompare();
  void testSpecialisedVectorLayouts();
  void testAdditionalVectorOperations();
  void testSpecialisedVectorMembers();

public:
  VectorTest();
  ~VectorTest() override = default;
  void run() override;
};
