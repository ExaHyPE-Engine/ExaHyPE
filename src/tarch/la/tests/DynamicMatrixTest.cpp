// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicMatrixTest.h"

#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/GramSchmidt.h"
#include "tarch/la/LUDecomposition.h"

tarch::la::tests::DynamicMatrixTest::DynamicMatrixTest():
  TestCase("tarch::la::tests::DynamicMatrixTest") {}

void tarch::la::tests::DynamicMatrixTest::run() {
  testMethod(testBatchedMultiplyAoS);
  testMethod(testInterpolationMatrixEditing);
  testMethod(testMultiplyAndTranspose);
  testMethod(testKroneckerConstruction);
  testMethod(testAccessorsAndCopies);
  testMethod(testGramSchmidtAndTriangularInverseWithSingularColumn);
}

void tarch::la::tests::DynamicMatrixTest::testBatchedMultiplyAoS() {
  tarch::la::DynamicMatrix P(
    24,
    8,
    {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0.666667, 0, 0.333333, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0.333333, 0, 0.666667, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0.666667, 0, 0.333333, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0.333333, 0, 0.666667, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0.666667, 0, 0.333333},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0.333333, 0, 0.666667},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 1},
     {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 1}}
  );

  double Q[] = {
    4.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0,
    4.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0
  };

  double result[5 * 4 * 2];

  P.batchedMultiplyAoS(result, Q, 5, 4 * 2, 0);
  validateEqualsWithParams1(result[0 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[1 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[2 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[3 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[4 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[5 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[6 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[7 * 5], 0.1, P.toString(true));

  P.batchedMultiplyAoS(result, Q, 5, 4 * 2, 8);
  validateEqualsWithParams1(result[0 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[1 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[2 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[3 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[4 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[5 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[6 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[7 * 5], 0.1, P.toString(true));

  P.batchedMultiplyAoS(result, Q, 5, 4 * 2, 16);
  validateEqualsWithParams1(result[0 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[1 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[2 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[3 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[4 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[5 * 5], 0.1, P.toString(true));
  validateEqualsWithParams1(result[6 * 5], 0.0, P.toString(true));
  validateEqualsWithParams1(result[7 * 5], 0.1, P.toString(true));
}

void tarch::la::tests::DynamicMatrixTest::testInterpolationMatrixEditing() {
  tarch::la::DynamicMatrix matrix(3, 1, {{1.0}, {1.0}, {1.0}});

  matrix.replicateRows(3, 4, 1, true);
  validateEquals(matrix.rows(), 12);
  validateEquals(matrix.cols(), 4);
  validateEquals(matrix(0, 0), 1.0);
  validateEquals(matrix(3, 1), 1.0);
  validateEquals(matrix(6, 2), 1.0);
  validateEquals(matrix(9, 3), 1.0);

  matrix.insertEmptyColumns(1, 1, 0);
  validateEquals(matrix.cols(), 5);
  validateEquals(matrix(0, 0), 1.0);
  validateEquals(matrix(0, 1), 0.0);
  validateEquals(matrix(3, 2), 1.0);

  matrix.removeColumn(0);
  validateEquals(matrix.cols(), 4);
  validateEquals(matrix(0, 0), 0.0);
  validateEquals(matrix(3, 1), 1.0);

  tarch::la::DynamicMatrix replicated(2, 2, {{1.0, 2.0}, {3.0, 4.0}});
  replicated.replicateCols(1, 2, 0, false);
  validateEquals(replicated.rows(), 2);
  validateEquals(replicated.cols(), 4);
  validateEquals(replicated(0, 0), 1.0);
  validateEquals(replicated(0, 1), 1.0);
  validateEquals(replicated(0, 2), 2.0);
  validateEquals(replicated(0, 3), 2.0);

  replicated.scale(0.5);
  validateEquals(replicated(0, 0), 0.5);
  validateEquals(replicated(0, 1), 0.5);
  validateEquals(replicated(0, 2), 1.0);
  validateEquals(replicated(0, 3), 1.0);

  tarch::la::DynamicMatrix columnBlocked(3, 2, {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
  columnBlocked.replicateCols(2, 2, 0, false);
  validateEquals(columnBlocked.rows(), 3);
  validateEquals(columnBlocked.cols(), 4);
  validateEquals(columnBlocked(0, 0), 1.0);
  validateEquals(columnBlocked(0, 1), 2.0);
  validateEquals(columnBlocked(0, 2), 1.0);
  validateEquals(columnBlocked(0, 3), 2.0);
}

void tarch::la::tests::DynamicMatrixTest::testMultiplyAndTranspose() {
  tarch::la::DynamicMatrix lhs(2, 3, {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  tarch::la::DynamicMatrix rhs(3, 2, {{6.0, 5.0}, {4.0, 3.0}, {2.0, 1.0}});

  tarch::la::DynamicMatrix product = lhs * rhs;
  validateEquals(product.rows(), 2);
  validateEquals(product.cols(), 2);
  validateEquals(product(0, 0), 20.0);
  validateEquals(product(0, 1), 14.0);
  validateEquals(product(1, 0), 56.0);
  validateEquals(product(1, 1), 41.0);

  tarch::la::DynamicMatrix transposeLhs = tarch::la::transpose(lhs);
  validateEquals(transposeLhs.rows(), 3);
  validateEquals(transposeLhs.cols(), 2);
  validateEquals(transposeLhs(0, 0), 1.0);
  validateEquals(transposeLhs(2, 1), 6.0);

  double result[4];
  lhs.multiplyBySmallMatrix(result, rhs);
  validateEquals(result[0], 20.0);
  validateEquals(result[1], 14.0);
  validateEquals(result[2], 56.0);
  validateEquals(result[3], 41.0);
}

void tarch::la::tests::DynamicMatrixTest::testKroneckerConstruction() {
  tarch::la::DynamicMatrix lhs(2, 2, {{1.0, 2.0}, {3.0, 4.0}});
  tarch::la::DynamicMatrix rhs(2, 1, {{5.0}, {6.0}});

  tarch::la::DynamicMatrix product(lhs, rhs);
  validateEquals(product.rows(), 4);
  validateEquals(product.cols(), 2);
  validateEquals(product(0, 0), 5.0);
  validateEquals(product(1, 0), 6.0);
  validateEquals(product(0, 1), 10.0);
  validateEquals(product(1, 1), 12.0);
  validateEquals(product(2, 0), 15.0);
  validateEquals(product(3, 1), 24.0);
}

void tarch::la::tests::DynamicMatrixTest::testAccessorsAndCopies() {
  tarch::la::DynamicMatrix matrix(2, 2, {{1.0, 2.0}, {3.0, 4.0}});
  validateEquals(matrix.rows(), 2);
  validateEquals(matrix.cols(), 2);
  validateEquals(matrix.data()[0], 1.0);
  matrix(1, 0) = 7.0;
  validateEquals(matrix(1, 0), 7.0);
  validate(!matrix.toString().empty());

  tarch::la::DynamicMatrix copied(matrix);
  validateEquals(copied(1, 0), 7.0);

  tarch::la::DynamicMatrix moved(std::move(copied));
  validateEquals(moved(1, 0), 7.0);

  double x[] = {1.0, 2.0, 3.0, 4.0};
  double result[4];
  matrix.batchedMultiplyAoS(result, x, 2);
  validateEquals(result[0], 7.0);
  validateEquals(result[1], 10.0);
  validateEquals(result[2], 19.0);
  validateEquals(result[3], 30.0);
}

void tarch::la::tests::DynamicMatrixTest::testGramSchmidtAndTriangularInverseWithSingularColumn() {
  tarch::la::DynamicMatrix A(3, 3, {{1.0, 2.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}});
  tarch::la::DynamicMatrix Q(3, 3);
  tarch::la::DynamicMatrix R(3, 3);

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

  tarch::la::DynamicMatrix reconstructed = Q * R;
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      validateNumericalEqualsWithParams2(reconstructed(row, col), A(row, col), row, col);
    }
  }

  tarch::la::DynamicMatrix inverseR = tarch::la::invertUpperTriangular(R);
  validateNumericalEquals(inverseR(0, 0), 1.0);
  validateNumericalEquals(inverseR(0, 1), -2.0);
  validateNumericalEquals(inverseR(1, 1), 1.0);
  validateNumericalEquals(inverseR(2, 2), 1.0);
}
