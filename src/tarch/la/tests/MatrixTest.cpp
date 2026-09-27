// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MatrixTest.h"

#include "tarch/la/Matrix.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixOperations.h"
#include "tarch/la/MatrixScalarOperations.h"

tarch::la::tests::MatrixTest::MatrixTest() :
  tarch::tests::TestCase("tarch::la::tests::MatrixTest") {}

void
tarch::la::tests::MatrixTest::run() {
  testMethod(testConstruction);
  testMethod(testAssignment);
  testMethod(testMatrixOperations);
  testMethod(testMatrixMatrixOperations);
  testMethod(testTransposedMatrix);
  testMethod(testSpecialisedMatrixLayouts);
  testMethod(testAdditionalMatrixOperations);
  testMethod(testNaNComparesUnequal);
  testMethod(testComponentProductOfRectangularMatrices);
}

void
tarch::la::tests::MatrixTest::testConstruction() {
  const tarch::la::Matrix<1, 2, int> matrix(1);
  validateEquals(matrix.getSize(), 2);
  validateEquals(matrix.getNumberOfRows(), 1);
  validateEquals(matrix.getNumberOfColumns(), 2);
  validateEquals(matrix(0, 0), 1);
  validateEquals(matrix(0, 1), 1);

  const tarch::la::Matrix<1, 2, int> matrix2(matrix);
  validateEquals(matrix2(0, 0), 1);
  validateEquals(matrix2(0, 1), 1);
}

void
tarch::la::tests::MatrixTest::testAssignment() {
  const tarch::la::Matrix<2, 2, int> matrix = {1, 2, 3, 4};
  validateEquals(matrix(0, 0), 1);
  validateEquals(matrix(0, 1), 2);
  validateEquals(matrix(1, 0), 3);
  validateEquals(matrix(1, 1), 4);

  const tarch::la::Matrix<1, 2, int> matrix2 = {1, 2};
  validateEquals(matrix2(0, 0), 1);
  validateEquals(matrix2(0, 1), 2);
}

void
tarch::la::tests::MatrixTest::testMatrixOperations() {
  tarch::la::Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
  std::ostringstream           stream;
  stream << matrix2;
  validateEquals(stream.str(), std::string("[[1,2],[3,4]]"));

  matrix2 = matrix2 * 2;
  validateEquals(matrix2(0, 0), 2);
  validateEquals(matrix2(0, 1), 4);
  validateEquals(matrix2(1, 0), 6);
  validateEquals(matrix2(1, 1), 8);
}

void
tarch::la::tests::MatrixTest::testMatrixMatrixOperations() {
  const tarch::la::Matrix<2, 3, int> lMatrix = {1, 2, 3, 4, 5, 6};
  const tarch::la::Matrix<3, 2, int> rMatrix = {6, 5, 4, 3, 2, 1};
  const tarch::la::Matrix<2, 2, int> result  = lMatrix * rMatrix;
  validateEquals(result(0, 0), 20);
  validateEquals(result(0, 1), 14);
  validateEquals(result(1, 0), 56);
  validateEquals(result(1, 1), 41);

  const tarch::la::Matrix<2, 3, int> matrixA(1);
  const tarch::la::Matrix<2, 3, int> matrixB(2);
  validate(matrixA == matrixA);
  validate(not(matrixA == matrixB));

  const tarch::la::Matrix<2, 3, double> matrix1(1);
  const tarch::la::Matrix<2, 3, double> matrix2(2);
  const std::pair<int, int>             index = tarch::la::equalsReturnIndex(matrix1, matrix2);
  validateEquals(index.first, 0);
  validateEquals(index.second, 0);

  const tarch::la::Matrix<2, 5, int> genericLeft    = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const tarch::la::Matrix<5, 3, int> genericRight   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  const tarch::la::Matrix<2, 3, int> genericProduct = genericLeft * genericRight;
  validateEquals(genericProduct(0, 0), 135);
  validateEquals(genericProduct(0, 1), 150);
  validateEquals(genericProduct(0, 2), 165);
  validateEquals(genericProduct(1, 0), 310);
  validateEquals(genericProduct(1, 1), 350);
  validateEquals(genericProduct(1, 2), 390);

  const tarch::la::Matrix<2, 5, int> genericA     = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const tarch::la::Matrix<2, 5, int> genericB     = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  const auto                         genericPlus  = genericA + genericB;
  const auto                         genericMinus = genericA - genericB;
  validateEquals(genericPlus(0, 0), 11);
  validateEquals(genericPlus(1, 4), 11);
  validateEquals(genericMinus(0, 0), -9);
  validateEquals(genericMinus(1, 4), 9);
}

void
tarch::la::tests::MatrixTest::testTransposedMatrix() {
  const tarch::la::Matrix<3, 2, int> matrix     = {1, 2, 3, 4, 5, 6};
  const tarch::la::Matrix<2, 3, int> transposed = tarch::la::transpose(matrix);
  validateEquals(transposed(0, 0), 1);
  validateEquals(transposed(0, 1), 3);
  validateEquals(transposed(0, 2), 5);
  validateEquals(transposed(1, 0), 2);
  validateEquals(transposed(1, 1), 4);
  validateEquals(transposed(1, 2), 6);

  validate(tarch::la::transpose(transposed) == matrix);
}

void
tarch::la::tests::MatrixTest::testSpecialisedMatrixLayouts() {
  static_assert(sizeof(tarch::la::Matrix<1, 1, int>) == sizeof(int));
  static_assert(std::is_trivially_copyable<tarch::la::Matrix<1, 1, int>>::value);
  static_assert(sizeof(tarch::la::Matrix<2, 2, int>) == 16);
  static_assert(alignof(tarch::la::Matrix<2, 2, int>) == 8);
  static_assert(std::is_standard_layout<tarch::la::Matrix<2, 2, int>>::value);
  static_assert(std::is_trivially_copyable<tarch::la::Matrix<2, 2, int>>::value);
  static_assert(sizeof(tarch::la::Matrix<3, 3, int>) == 48);
  static_assert(alignof(tarch::la::Matrix<3, 3, int>) == 16);
  static_assert(std::is_standard_layout<tarch::la::Matrix<3, 3, int>>::value);
  static_assert(std::is_trivially_copyable<tarch::la::Matrix<3, 3, int>>::value);
  static_assert(sizeof(tarch::la::Matrix<4, 4, int>) == 64);
  static_assert(alignof(tarch::la::Matrix<4, 4, int>) == 16);
  static_assert(std::is_standard_layout<tarch::la::Matrix<4, 4, int>>::value);
  static_assert(std::is_trivially_copyable<tarch::la::Matrix<4, 4, int>>::value);

  const tarch::la::Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
  validateEquals(matrix2.getNumberOfRows(), 2);
  validateEquals(matrix2.getNumberOfColumns(), 2);
  validateEquals(matrix2.getData()[0], 1);
  validateEquals(matrix2.getData()[3], 4);

  const tarch::la::Matrix<3, 3, int> matrix3(1);
  validateEquals(matrix3.getSize(), 9);
  validateEquals(matrix3(0, 0), 1);
  validateEquals(matrix3(2, 2), 1);

  const tarch::la::Matrix<4, 4, int> matrix4(2);
  validateEquals(matrix4.getSize(), 16);
  validateEquals(matrix4(0, 0), 2);
  validateEquals(matrix4(3, 3), 2);

  const tarch::la::Matrix<2, 3, int> generic   = {1, 2, 3, 4, 5, 6};
  const auto                         converted = generic.convertScalar<double>();
  validateEquals(converted.getNumberOfRows(), 2);
  validateEquals(converted.getNumberOfColumns(), 3);
  validateEquals(converted(0, 0), 1.0);
  validateEquals(converted(1, 2), 6.0);
}

void
tarch::la::tests::MatrixTest::testAdditionalMatrixOperations() {
  const int  raw[] = {1, 2, 3, 4, 5, 6};
  const auto built = tarch::la::buildMatrix<2, 3, int>(raw);
  validateEquals(built(0, 0), 1);
  validateEquals(built(1, 2), 6);

  const tarch::la::Matrix<3, 3, double> identityMatrix = tarch::la::identity<3, double>();
  validateEquals(identityMatrix(0, 0), 1.0);
  validateEquals(identityMatrix(1, 2), 0.0);

  const tarch::la::Matrix<2, 3, double> matrix = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  validateEquals(tarch::la::sum(matrix), 21.0);
  validateNumericalEquals(tarch::la::frobeniusNorm(matrix), std::sqrt(91.0));
  validateEquals(tarch::la::elementMax(tarch::la::Matrix<2, 2, double>{-1.0, 2.5, -7.0, 4.0}), 7.0);

  const tarch::la::Matrix<2, 2, double> invEntries =
    tarch::la::invertEntries(tarch::la::Matrix<2, 2, double>{1.0, 2.0, 4.0, 8.0});
  validateNumericalEquals(invEntries(0, 0), 1.0);
  validateNumericalEquals(invEntries(0, 1), 0.5);
  validateNumericalEquals(invEntries(1, 0), 0.25);
  validateNumericalEquals(invEntries(1, 1), 0.125);

  const auto diagonal = tarch::la::getDiagonal(tarch::la::Matrix<3, 3, int>{1, 2, 3, 4, 5, 6, 7, 8, 9});
  validateEquals(diagonal[0], 1);
  validateEquals(diagonal[1], 5);
  validateEquals(diagonal[2], 9);

  const auto matrixRow = tarch::la::getRow(matrix, 1);
  const auto matrixCol = tarch::la::getColumn(matrix, 2);
  validateEquals(matrixRow[0], 4.0);
  validateEquals(matrixRow[2], 6.0);
  validateEquals(matrixCol[0], 3.0);
  validateEquals(matrixCol[1], 6.0);

  tarch::la::Matrix<2, 2, int> scaled = {1, 2, 3, 4};
  scaled *= 3;
  validateEquals(scaled(0, 0), 3);
  validateEquals(scaled(1, 1), 12);
  scaled = 2 * scaled;
  validateEquals(scaled(0, 1), 12);
  validateEquals(scaled(1, 0), 18);

  tarch::la::Matrix<3, 3, double> scaled3 = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
  scaled3 *= 0.5;
  validateNumericalEquals(scaled3(0, 0), 0.5);
  validateNumericalEquals(scaled3(2, 2), 4.5);
  const auto scaled3Copy = scaled3 * 2.0;
  validateNumericalEquals(scaled3Copy(1, 1), 5.0);
  validateNumericalEquals(tarch::la::sum(scaled3Copy), 45.0);
  validateNumericalEquals(tarch::la::frobeniusNorm(scaled3Copy), std::sqrt(285.0));

  const tarch::la::Matrix<2, 2, int> lhs              = {1, 2, 3, 4};
  const tarch::la::Matrix<2, 2, int> rhs              = {4, 3, 2, 1};
  const auto                         plusResult       = lhs + rhs;
  const auto                         minusResult      = lhs - rhs;
  const auto                         componentProduct = tarch::la::multiplyComponents(lhs, rhs);
  validateEquals(plusResult(0, 0), 5);
  validateEquals(minusResult(1, 1), 3);
  validateEquals(componentProduct(0, 0), 4);
  validateEquals(componentProduct(1, 1), 4);
  validate(tarch::la::equals(lhs, lhs));

  const tarch::la::Matrix<3, 3, int> lhs3       = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  const tarch::la::Matrix<3, 3, int> rhs3       = {9, 8, 7, 6, 5, 4, 3, 2, 1};
  const auto                         plus3      = lhs3 + rhs3;
  const auto                         minus3     = lhs3 - rhs3;
  const auto                         component3 = tarch::la::multiplyComponents(lhs3, rhs3);
  validateEquals(plus3(0, 0), 10);
  validateEquals(plus3(2, 2), 10);
  validateEquals(minus3(0, 0), -8);
  validateEquals(minus3(2, 2), 8);
  validateEquals(component3(1, 1), 25);
  validate(tarch::la::equals(lhs3, lhs3));
  validate(not(lhs3 == rhs3));

  const tarch::la::Matrix<4, 4, int> matrix4Specialised = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  const auto                         diagonal4          = tarch::la::getDiagonal(matrix4Specialised);
  validateEquals(diagonal4[0], 1);
  validateEquals(diagonal4[1], 6);
  validateEquals(diagonal4[2], 11);
  validateEquals(diagonal4[3], 16);
  const auto row4 = tarch::la::getRow(matrix4Specialised, 2);
  const auto col4 = tarch::la::getColumn(matrix4Specialised, 1);
  validateEquals(row4[0], 9);
  validateEquals(row4[3], 12);
  validateEquals(col4[0], 2);
  validateEquals(col4[3], 14);
  const auto transpose4 = tarch::la::transpose(matrix4Specialised);
  validateEquals(transpose4(0, 3), 13);
  validateEquals(transpose4(3, 0), 4);
  validateEquals(tarch::la::sum(matrix4Specialised), 136);
  validateEquals(
    tarch::la::elementMax(tarch::la::Matrix<4, 4, int>{-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16}),
    16
  );

  const tarch::la::Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
  const tarch::la::Matrix<3, 3, int> matrix3 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  const tarch::la::Matrix<4, 4, int> matrix4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  validate(not matrix2.toPrettyString().empty());
  validate(not matrix2.toString().empty());
  validate(not matrix3.toPrettyString().empty());
  validate(not matrix4.toPrettyString().empty());

  const auto converted3 = tarch::la::Matrix<3, 3, int>{1, 2, 3, 4, 5, 6, 7, 8, 9}.convertScalar<double>();
  const auto converted4 =
    tarch::la::Matrix<4, 4, int>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}.convertScalar<double>();
  validateEquals(converted3(2, 1), 8.0);
  validateEquals(converted4(3, 3), 1.0);

  const tarch::la::Matrix<5, 5, double> generic5 = {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,
                                                    10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0,
                                                    19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0};
  validateEquals(tarch::la::sum(generic5), 325.0);
  validateNumericalEquals(tarch::la::frobeniusNorm(generic5), std::sqrt(5525.0));
  validateEquals(tarch::la::elementMax(generic5), 25.0);

  const auto generic5Transpose = tarch::la::transpose(generic5);
  validateEquals(generic5Transpose(0, 4), 21.0);
  validateEquals(generic5Transpose(4, 0), 5.0);

  const auto generic5Diag = tarch::la::getDiagonal(generic5);
  validateEquals(generic5Diag[0], 1.0);
  validateEquals(generic5Diag[2], 13.0);
  validateEquals(generic5Diag[4], 25.0);

  const auto generic5Row = tarch::la::getRow(generic5, 3);
  const auto generic5Col = tarch::la::getColumn(generic5, 1);
  validateEquals(generic5Row[0], 16.0);
  validateEquals(generic5Row[4], 20.0);
  validateEquals(generic5Col[0], 2.0);
  validateEquals(generic5Col[4], 22.0);

  const auto generic5Scaled = generic5 * 0.5;
  validateEquals(generic5Scaled(0, 0), 0.5);
  validateEquals(generic5Scaled(4, 4), 12.5);

  const tarch::la::Matrix<5, 5, double> genericInv = tarch::la::invertEntries(
    tarch::la::Matrix<5, 5, double>{1.0,    2.0,     4.0,     5.0,     10.0,    20.0,    25.0,    50.0,   100.0,
                                    125.0,  200.0,   250.0,   400.0,   500.0,   1000.0,  2000.0,  2500.0, 4000.0,
                                    5000.0, 10000.0, 20000.0, 25000.0, 40000.0, 50000.0, 100000.0}
  );
  validateNumericalEquals(genericInv(0, 0), 1.0);
  validateNumericalEquals(genericInv(0, 1), 0.5);
  validateNumericalEquals(genericInv(4, 4), 0.00001);

  const tarch::la::Matrix<6, 4, int> rectangular          = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
  const auto                         rectangularTranspose = tarch::la::transpose(rectangular);
  validateEquals(rectangularTranspose(0, 5), 21);
  validateEquals(rectangularTranspose(3, 0), 4);
}

/** A NaN entry makes every tolerance comparison of matrices report a difference, whatever the tolerance. */
void
tarch::la::tests::MatrixTest::testNaNComparesUnequal() {
#if not defined(PEANO_WITH_FPE)
  // A build with floating-point traps aborts at an ordered comparison with a NaN.
  constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();
  constexpr double Tolerance  = std::numeric_limits<double>::max();

  const tarch::la::Matrix<2, 3, double> finite  = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  const tarch::la::Matrix<2, 3, double> withNaN = {1.0, 2.0, 3.0, 4.0, NotANumber, 6.0};
  validate(tarch::la::equals(finite, finite, Tolerance));
  validate(not tarch::la::equals(finite, withNaN, Tolerance));
  const std::pair<int, int> index = tarch::la::equalsReturnIndex(finite, withNaN, Tolerance);
  validateEquals(index.first, 1);
  validateEquals(index.second, 1);

  const tarch::la::Matrix<2, 2, double> square2 = {1.0, NotANumber, 3.0, 4.0};
  const tarch::la::Matrix<3, 3, double> square3 = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, NotANumber};
  validate(not tarch::la::equals(square2, square2, Tolerance));
  validate(not tarch::la::equals(square3, square3, Tolerance));
#endif
}

/** The component product of two matrices of the same shape multiplies the entries at equal positions. */
void
tarch::la::tests::MatrixTest::testComponentProductOfRectangularMatrices() {
  const tarch::la::Matrix<2, 3, int> lhs     = {1, 2, 3, 4, 5, 6};
  const tarch::la::Matrix<2, 3, int> rhs     = {6, 5, 4, 3, 2, 1};
  const tarch::la::Matrix<2, 3, int> product = tarch::la::multiplyComponents(lhs, rhs);
  validateEquals(product(0, 0), 6);
  validateEquals(product(0, 1), 10);
  validateEquals(product(0, 2), 12);
  validateEquals(product(1, 0), 12);
  validateEquals(product(1, 1), 10);
  validateEquals(product(1, 2), 6);
}
