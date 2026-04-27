// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GramSchmidtTest.h"

#include "tarch/la/GramSchmidt.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/ScalarOperations.h"

tarch::la::tests::GramSchmidtTest::GramSchmidtTest():
  TestCase("tarch::la::tests::GramSchmidtTest") {}

void tarch::la::tests::GramSchmidtTest::run() {
  testMethod(testModifiedGramSchmidt);
  testMethod(testDynamicModifiedGramSchmidt);
  testMethod(testModifiedGramSchmidtWithSingularColumn);
}

void tarch::la::tests::GramSchmidtTest::testModifiedGramSchmidt() {
  Matrix<4, 4, double> A;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double entry = 1.0 / static_cast<double>(i + j + 1);
      A(i, j)      = entry;
    }
  }

  Matrix<4, 4, double> Acopy(A);
  Matrix<4, 4, double> Q;
  Matrix<4, 4, double> R;
  modifiedGramSchmidt(Acopy, Q, R);

  Matrix<4, 4, double> QTQ(0.0);
  QTQ = multiply(transpose(Q), Q);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        validate(equals(QTQ(i, j), 1.0, 1e-12));
      } else {
        validate(equals(QTQ(i, j), 0.0, 1e-12));
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double value = 0.0;
      for (int k = 0; k < Q.cols(); k++) {
        value += Q(i, k) * R(k, j);
      }
      validate(equals(value, A(i, j)));
      value = 0.0;
    }
  }
}

void tarch::la::tests::GramSchmidtTest::testDynamicModifiedGramSchmidt() {
  DynamicMatrix A(3, 3, {{1.0, 1.0, 0.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}});
  DynamicMatrix Q(3, 3);
  DynamicMatrix R(3, 3);

  modifiedGramSchmidt(A, Q, R);

  DynamicMatrix reconstructed = Q * R;
  validateNumericalEquals(reconstructed(0, 0), 1.0);
  validateNumericalEquals(reconstructed(0, 1), 1.0);
  validateNumericalEquals(reconstructed(0, 2), 0.0);
  validateNumericalEquals(reconstructed(1, 0), 1.0);
  validateNumericalEquals(reconstructed(1, 1), 0.0);
  validateNumericalEquals(reconstructed(1, 2), 1.0);
  validateNumericalEquals(reconstructed(2, 0), 0.0);
  validateNumericalEquals(reconstructed(2, 1), 1.0);
  validateNumericalEquals(reconstructed(2, 2), 1.0);
}

void tarch::la::tests::GramSchmidtTest::testModifiedGramSchmidtWithSingularColumn() {
  Matrix<3, 3, double> A = {1.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0};
  Matrix<3, 3, double> Q(0.0);
  Matrix<3, 3, double> R(0.0);

  modifiedGramSchmidt(A, Q, R);

  validateNumericalEquals(R(0, 0), 1.0);
  validateNumericalEquals(R(1, 1), 1.0);
  validateNumericalEquals(R(2, 2), 0.0);

  validateNumericalEquals(Q(0, 0), 1.0);
  validateNumericalEquals(Q(1, 0), 0.0);
  validateNumericalEquals(Q(2, 0), 0.0);

  validateNumericalEquals(Q(0, 1), 0.0);
  validateNumericalEquals(Q(1, 1), 1.0);
  validateNumericalEquals(Q(2, 1), 0.0);

  validateNumericalEquals(Q(0, 2), 0.0);
  validateNumericalEquals(Q(1, 2), 0.0);
  validateNumericalEquals(Q(2, 2), 0.0);

  Matrix<3, 3, double> reconstructed = Q * R;
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      validateNumericalEqualsWithParams2(reconstructed(row, col), A(row, col), row, col);
    }
  }
}
