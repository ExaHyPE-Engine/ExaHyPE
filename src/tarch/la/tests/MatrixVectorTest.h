// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class MatrixVectorTest;
} // namespace tarch::la::tests

class tarch::la::tests::MatrixVectorTest final : public tarch::tests::TestCase {
private:
  void testMultiplication();
  void testAdditionalMatrixVectorOperations();

public:
  MatrixVectorTest();
  ~MatrixVectorTest() override = default;
  void run() override;
};
