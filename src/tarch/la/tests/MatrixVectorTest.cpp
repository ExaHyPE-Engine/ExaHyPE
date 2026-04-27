// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MatrixVectorTest.h"

#include "tarch/la/Matrix.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixVectorOperations.h"
#include "tarch/la/Vector.h"

tarch::la::tests::MatrixVectorTest::MatrixVectorTest():
  TestCase("tarch::la::tests::MatrixVectorTest") {}

void tarch::la::tests::MatrixVectorTest::run() {
  testMethod(testMultiplication);
  testMethod(testAdditionalMatrixVectorOperations);
}

void tarch::la::tests::MatrixVectorTest::testMultiplication() {
  Matrix<2, 2, int> matrix;
  Vector<2, int>    vector;
  matrix = {1, 2, 3, 4};
  vector = {1, 2};

  Vector<2, int> result(0);
  result = multiply(matrix, vector);
  validateEquals(result[0], 5);
  validateEquals(result[1], 11);
}

void tarch::la::tests::MatrixVectorTest::testAdditionalMatrixVectorOperations() {
  Matrix<2, 2, double> matrix = {1.0, 2.0, 3.0, 4.0};
  Vector<2, double>    vector = {1.5, -0.5};

  auto result = matrix * vector;
  validateNumericalEquals(result[0], 0.5);
  validateNumericalEquals(result[1], 2.5);

  Vector<2, double> accum(1.0);
  matVec(matrix, 2.0, vector, accum);
  validateNumericalEquals(accum[0], 2.0);
  validateNumericalEquals(accum[1], 6.0);

  auto outer = outerDot(Vector<2, double>{1.0, 2.0}, Vector<2, double>{3.0, 4.0});
  validateNumericalEquals(outer(0, 0), 3.0);
  validateNumericalEquals(outer(1, 0), 6.0);
  validateNumericalEquals(outer(0, 1), 4.0);
  validateNumericalEquals(outer(1, 1), 8.0);
}
