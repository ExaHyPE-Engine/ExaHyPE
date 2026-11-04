// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class MatrixTest;
} // namespace tarch::la::tests

class tarch::la::tests::MatrixTest final : public tarch::tests::TestCase {
public:
  MatrixTest();
  MatrixTest(const MatrixTest&)            = delete;
  MatrixTest(MatrixTest&&)                 = delete;
  MatrixTest& operator=(const MatrixTest&) = delete;
  MatrixTest& operator=(MatrixTest&&)      = delete;
  ~MatrixTest() override                   = default;

  void run() override;

private:
  void testConstruction();
  void testAssignment();
  void testMatrixOperations();
  void testMatrixMatrixOperations();
  void testTransposedMatrix();
  void testSpecialisedMatrixLayouts();
  void testAdditionalMatrixOperations();
  void testNaNComparesUnequal();
  void testComponentProductOfRectangularMatrices();
};
