// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MatrixTest.h"

#include "tarch/la/Matrix.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixOperations.h"
#include "tarch/la/MatrixScalarOperations.h"

#include <sstream>
#include <string>

tarch::la::tests::MatrixTest::MatrixTest():
  TestCase("tarch::la::tests::MatrixTest") {}

void tarch::la::tests::MatrixTest::run() {
  testMethod(testConstruction);
  testMethod(testAssignment);
  testMethod(testMatrixOperations);
  testMethod(testMatrixMatrixOperations);
  testMethod(testTransposedMatrix);
  testMethod(testSpecialisedMatrixLayouts);
  testMethod(testAdditionalMatrixOperations);
}

void tarch::la::tests::MatrixTest::testConstruction() {
  Matrix<1, 2, int> matrix(1);
  validateEquals(matrix.size(), 2);
  validateEquals(matrix.rows(), 1);
  validateEquals(matrix.cols(), 2);
  validateEquals(matrix(0, 0), 1);
  validateEquals(matrix(0, 1), 1);

  Matrix<1, 2, int> matrix2(matrix);
  validateEquals(matrix2(0, 0), 1);
  validateEquals(matrix2(0, 1), 1);
}

void tarch::la::tests::MatrixTest::testAssignment() {
  Matrix<2, 2, int> matrix = {1, 2, 3, 4};
  validateEquals(matrix(0, 0), 1);
  validateEquals(matrix(0, 1), 2);
  validateEquals(matrix(1, 0), 3);
  validateEquals(matrix(1, 1), 4);

  Matrix<1, 2, int> matrix2 = {1, 2};
  validateEquals(matrix2(0, 0), 1);
  validateEquals(matrix2(0, 1), 2);
}

void tarch::la::tests::MatrixTest::testMatrixOperations() {
  // Test streaming
  Matrix<2, 2, int>  matrix2 = {1, 2, 3, 4};
  std::ostringstream stream;
  stream << matrix2;
  validateEquals(stream.str(), std::string("[[1,2],[3,4]]"));
  // Test matrix multiply scalar
  matrix2 = matrix2 * 2;
  validateEquals(matrix2(0, 0), 2);
  validateEquals(matrix2(0, 1), 4);
  validateEquals(matrix2(1, 0), 6);
  validateEquals(matrix2(1, 1), 8);
}

void tarch::la::tests::MatrixTest::testMatrixMatrixOperations() {
  Matrix<2, 3, int> lMatrix;
  Matrix<3, 2, int> rMatrix;
  Matrix<2, 2, int> result(0);
  lMatrix = {1, 2, 3, 4, 5, 6};
  rMatrix = {6, 5, 4, 3, 2, 1};

  // Matrix matrix multiplication
  result = lMatrix * rMatrix;
  validateEquals(result(0, 0), 20);
  validateEquals(result(0, 1), 14);
  validateEquals(result(1, 0), 56);
  validateEquals(result(1, 1), 41);

  // Bitwise comparison
  Matrix<2, 3, int> matrixA(1);
  Matrix<2, 3, int> matrixB(2);
  validate(matrixA == matrixA);
  validate(!(matrixA == matrixB));

  // Test equalsReturnIndex
  Matrix<2, 3, double> matrix1(1);
  Matrix<2, 3, double> matrix2(2);
  std::pair<int, int>  i = equalsReturnIndex(matrix1, matrix2);
  validateEquals(i.first, 0);
  validateEquals(i.second, 0);

  Matrix<2, 5, int> genericLeft    = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  Matrix<5, 3, int> genericRight   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  Matrix<2, 3, int> genericProduct = genericLeft * genericRight;
  validateEquals(genericProduct(0, 0), 135);
  validateEquals(genericProduct(0, 1), 150);
  validateEquals(genericProduct(0, 2), 165);
  validateEquals(genericProduct(1, 0), 310);
  validateEquals(genericProduct(1, 1), 350);
  validateEquals(genericProduct(1, 2), 390);

  Matrix<2, 5, int> genericA     = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  Matrix<2, 5, int> genericB     = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  auto              genericPlus  = genericA + genericB;
  auto              genericMinus = genericA - genericB;
  validateEquals(genericPlus(0, 0), 11);
  validateEquals(genericPlus(1, 4), 11);
  validateEquals(genericMinus(0, 0), -9);
  validateEquals(genericMinus(1, 4), 9);
}

void tarch::la::tests::MatrixTest::testTransposedMatrix() {
  Matrix<3, 2, int> matrix     = {1, 2, 3, 4, 5, 6};
  Matrix<2, 3, int> transposed = transpose(matrix);
  validateEquals(transposed(0, 0), 1);
  validateEquals(transposed(0, 1), 3);
  validateEquals(transposed(0, 2), 5);
  validateEquals(transposed(1, 0), 2);
  validateEquals(transposed(1, 1), 4);
  validateEquals(transposed(1, 2), 6);

  validate(transpose(transposed) == matrix);
}

void tarch::la::tests::MatrixTest::testSpecialisedMatrixLayouts() {
  Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
  validateEquals(matrix2.rows(), 2);
  validateEquals(matrix2.cols(), 2);
  validateEquals(matrix2.data()[0], 1);
  validateEquals(matrix2.data()[3], 4);

  Matrix<3, 3, int> matrix3(1);
  validateEquals(matrix3.size(), 9);
  validateEquals(matrix3(0, 0), 1);
  validateEquals(matrix3(2, 2), 1);

  Matrix<4, 4, int> matrix4(2);
  validateEquals(matrix4.size(), 16);
  validateEquals(matrix4(0, 0), 2);
  validateEquals(matrix4(3, 3), 2);

  Matrix<2, 3, int> generic   = {1, 2, 3, 4, 5, 6};
  auto              converted = generic.convertScalar<double>();
  validateEquals(converted.rows(), 2);
  validateEquals(converted.cols(), 3);
  validateEquals(converted(0, 0), 1.0);
  validateEquals(converted(1, 2), 6.0);
}

void tarch::la::tests::MatrixTest::testAdditionalMatrixOperations() {
  int  raw[] = {1, 2, 3, 4, 5, 6};
  auto built = buildMatrix<2, 3, int>(raw);
  validateEquals(built(0, 0), 1);
  validateEquals(built(1, 2), 6);

  Matrix<3, 3, double> identityMatrix = identity<3, double>();
  validateEquals(identityMatrix(0, 0), 1.0);
  validateEquals(identityMatrix(1, 2), 0.0);

  Matrix<2, 3, double> matrix = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  validateEquals(sum(matrix), 21.0);
  validateNumericalEquals(frobeniusNorm(matrix), std::sqrt(91.0));
  validateEquals(elementMax(Matrix<2, 2, double>{-1.0, 2.5, -7.0, 4.0}), 7.0);

  Matrix<2, 2, double> invEntries = invertEntries(Matrix<2, 2, double>{1.0, 2.0, 4.0, 8.0});
  validateNumericalEquals(invEntries(0, 0), 1.0);
  validateNumericalEquals(invEntries(0, 1), 0.5);
  validateNumericalEquals(invEntries(1, 0), 0.25);
  validateNumericalEquals(invEntries(1, 1), 0.125);

  auto diagonal = diag(Matrix<3, 3, int>{1, 2, 3, 4, 5, 6, 7, 8, 9});
  validateEquals(diagonal[0], 1);
  validateEquals(diagonal[1], 5);
  validateEquals(diagonal[2], 9);

  auto matrixRow = row(matrix, 1);
  auto matrixCol = col(matrix, 2);
  validateEquals(matrixRow[0], 4.0);
  validateEquals(matrixRow[2], 6.0);
  validateEquals(matrixCol[0], 3.0);
  validateEquals(matrixCol[1], 6.0);

  Matrix<2, 2, int> scaled = {1, 2, 3, 4};
  scaled *= 3;
  validateEquals(scaled(0, 0), 3);
  validateEquals(scaled(1, 1), 12);
  scaled = 2 * scaled;
  validateEquals(scaled(0, 1), 12);
  validateEquals(scaled(1, 0), 18);

  Matrix<3, 3, double> scaled3 = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
  scaled3 *= 0.5;
  validateNumericalEquals(scaled3(0, 0), 0.5);
  validateNumericalEquals(scaled3(2, 2), 4.5);
  auto scaled3Copy = scaled3 * 2.0;
  validateNumericalEquals(scaled3Copy(1, 1), 5.0);
  validateNumericalEquals(sum(scaled3Copy), 45.0);
  validateNumericalEquals(frobeniusNorm(scaled3Copy), std::sqrt(285.0));

  Matrix<2, 2, int> lhs              = {1, 2, 3, 4};
  Matrix<2, 2, int> rhs              = {4, 3, 2, 1};
  auto              plusResult       = lhs + rhs;
  auto              minusResult      = lhs - rhs;
  auto              componentProduct = multiplyComponents(lhs, rhs);
  validateEquals(plusResult(0, 0), 5);
  validateEquals(minusResult(1, 1), 3);
  validateEquals(componentProduct(0, 0), 4);
  validateEquals(componentProduct(1, 1), 4);
  validate(equals(lhs, lhs));

  Matrix<3, 3, int> lhs3       = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  Matrix<3, 3, int> rhs3       = {9, 8, 7, 6, 5, 4, 3, 2, 1};
  auto              plus3      = lhs3 + rhs3;
  auto              minus3     = lhs3 - rhs3;
  auto              component3 = multiplyComponents(lhs3, rhs3);
  validateEquals(plus3(0, 0), 10);
  validateEquals(plus3(2, 2), 10);
  validateEquals(minus3(0, 0), -8);
  validateEquals(minus3(2, 2), 8);
  validateEquals(component3(1, 1), 25);
  validate(equals(lhs3, lhs3));
  validate(!(lhs3 == rhs3));

  Matrix<4, 4, int> matrix4Specialised = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  auto              diagonal4          = diag(matrix4Specialised);
  validateEquals(diagonal4[0], 1);
  validateEquals(diagonal4[1], 6);
  validateEquals(diagonal4[2], 11);
  validateEquals(diagonal4[3], 16);
  auto row4 = row(matrix4Specialised, 2);
  auto col4 = col(matrix4Specialised, 1);
  validateEquals(row4[0], 9);
  validateEquals(row4[3], 12);
  validateEquals(col4[0], 2);
  validateEquals(col4[3], 14);
  auto transpose4 = transpose(matrix4Specialised);
  validateEquals(transpose4(0, 3), 13);
  validateEquals(transpose4(3, 0), 4);
  validateEquals(sum(matrix4Specialised), 136);
  validateEquals(elementMax(Matrix<4, 4, int>{-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16}), 16);

  Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
  Matrix<3, 3, int> matrix3 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  Matrix<4, 4, int> matrix4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  validate(!matrix2.toPrettyString().empty());
  validate(!matrix2.toString().empty());
  validate(!matrix3.toPrettyString().empty());
  validate(!matrix4.toPrettyString().empty());

  auto converted3 = Matrix<3, 3, int>{1, 2, 3, 4, 5, 6, 7, 8, 9}.convertScalar<double>();
  auto converted4 = Matrix<4, 4, int>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}.convertScalar<double>();
  validateEquals(converted3(2, 1), 8.0);
  validateEquals(converted4(3, 3), 1.0);

  Matrix<5, 5, double> generic5 = {
    1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0, 13.0,
    14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0
  };
  validateEquals(sum(generic5), 325.0);
  validateNumericalEquals(frobeniusNorm(generic5), std::sqrt(5525.0));
  validateEquals(elementMax(generic5), 25.0);

  auto generic5Transpose = transpose(generic5);
  validateEquals(generic5Transpose(0, 4), 21.0);
  validateEquals(generic5Transpose(4, 0), 5.0);

  auto generic5Diag = diag(generic5);
  validateEquals(generic5Diag[0], 1.0);
  validateEquals(generic5Diag[2], 13.0);
  validateEquals(generic5Diag[4], 25.0);

  auto generic5Row = row(generic5, 3);
  auto generic5Col = col(generic5, 1);
  validateEquals(generic5Row[0], 16.0);
  validateEquals(generic5Row[4], 20.0);
  validateEquals(generic5Col[0], 2.0);
  validateEquals(generic5Col[4], 22.0);

  auto generic5Scaled = generic5 * 0.5;
  validateEquals(generic5Scaled(0, 0), 0.5);
  validateEquals(generic5Scaled(4, 4), 12.5);

  Matrix<5, 5, double> genericInv = invertEntries(
    Matrix<5, 5, double>{
      1.0,   2.0,    4.0,    5.0,    10.0,   20.0,   25.0,    50.0,    100.0,   125.0,   200.0,   250.0,   400.0,
      500.0, 1000.0, 2000.0, 2500.0, 4000.0, 5000.0, 10000.0, 20000.0, 25000.0, 40000.0, 50000.0, 100000.0
    }
  );
  validateNumericalEquals(genericInv(0, 0), 1.0);
  validateNumericalEquals(genericInv(0, 1), 0.5);
  validateNumericalEquals(genericInv(4, 4), 0.00001);

  Matrix<6, 4, int> rectangular = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
  };
  auto rectangularTranspose = transpose(rectangular);
  validateEquals(rectangularTranspose(0, 5), 21);
  validateEquals(rectangularTranspose(3, 0), 4);
}
