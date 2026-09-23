// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class MatrixTest;
} // namespace tarch::la::tests

class tarch::la::tests::MatrixTest final : public tarch::tests::TestCase {
private:
  void testConstruction();
  void testAssignment();
  void testMatrixOperations();
  void testMatrixMatrixOperations();
  void testTransposedMatrix();
  void testSpecialisedMatrixLayouts();
  void testAdditionalMatrixOperations();

public:
  MatrixTest();
  ~MatrixTest() override = default;
  void run() override;
};
