// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LUDecompositionTest.h"

#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/LUDecomposition.h"

tarch::la::tests::LUDecompositionTest::LUDecompositionTest():
  TestCase("tarch::la::tests::LUDecompositionTest") {}

void tarch::la::tests::LUDecompositionTest::run() {
  testMethod(testBackSubstitution);
  testMethod(testInvertUpperTriangular);
  testMethod(testStaticBackSubstitution);
  testMethod(testStaticInvertUpperTriangular);
}

void tarch::la::tests::LUDecompositionTest::testBackSubstitution() {
  DynamicMatrix R(3, 3, {{2.0, 1.0, -1.0}, {0.0, 3.0, 2.0}, {0.0, 0.0, 4.0}});

  double rhs[3] = {2.0, 5.0, 8.0};
  double x[3]   = {0.0, 0.0, 0.0};

  tarch::la::backSubstitution(R, rhs, x);

  validateNumericalEquals(x[2], 2.0);
  validateNumericalEquals(x[1], 1.0 / 3.0);
  validateNumericalEquals(x[0], 11.0 / 6.0);
}

void tarch::la::tests::LUDecompositionTest::testInvertUpperTriangular() {
  DynamicMatrix R(3, 3, {{2.0, 1.0, -1.0}, {0.0, 3.0, 2.0}, {0.0, 0.0, 4.0}});

  DynamicMatrix inverseR = tarch::la::invertUpperTriangular(R);
  DynamicMatrix identity = R * inverseR;

  validateNumericalEquals(identity(0, 0), 1.0);
  validateNumericalEquals(identity(1, 1), 1.0);
  validateNumericalEquals(identity(2, 2), 1.0);
  validateNumericalEquals(identity(0, 1), 0.0);
  validateNumericalEquals(identity(0, 2), 0.0);
  validateNumericalEquals(identity(1, 2), 0.0);
}

void tarch::la::tests::LUDecompositionTest::testStaticBackSubstitution() {
  Matrix<3, 3, double> R   = {2.0, 1.0, -1.0, 0.0, 3.0, 2.0, 0.0, 0.0, 4.0};
  Vector<3, double>    rhs = {2.0, 5.0, 8.0};
  Vector<3, double>    x(0.0);

  tarch::la::backSubstitution(R, rhs, x);

  validateNumericalEquals(x[2], 2.0);
  validateNumericalEquals(x[1], 1.0 / 3.0);
  validateNumericalEquals(x[0], 11.0 / 6.0);
}

void tarch::la::tests::LUDecompositionTest::testStaticInvertUpperTriangular() {
  Matrix<3, 3, double> R = {2.0, 1.0, -1.0, 0.0, 3.0, 2.0, 0.0, 0.0, 4.0};

  Matrix<3, 3, double> inverseR = tarch::la::invertUpperTriangular(R);
  Matrix<3, 3, double> identity = R * inverseR;

  validateNumericalEquals(identity(0, 0), 1.0);
  validateNumericalEquals(identity(1, 1), 1.0);
  validateNumericalEquals(identity(2, 2), 1.0);
  validateNumericalEquals(identity(0, 1), 0.0);
  validateNumericalEquals(identity(0, 2), 0.0);
  validateNumericalEquals(identity(1, 2), 0.0);
}
