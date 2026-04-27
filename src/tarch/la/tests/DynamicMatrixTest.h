// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class DynamicMatrixTest;
} // namespace tarch::la::tests

class tarch::la::tests::DynamicMatrixTest final: public tarch::tests::TestCase {
private:
  void testBatchedMultiplyAoS();
  void testInterpolationMatrixEditing();
  void testMultiplyAndTranspose();
  void testKroneckerConstruction();
  void testAccessorsAndCopies();
  void testGramSchmidtAndTriangularInverseWithSingularColumn();

public:
  DynamicMatrixTest();
  ~DynamicMatrixTest() override = default;
  void run() override;
};
