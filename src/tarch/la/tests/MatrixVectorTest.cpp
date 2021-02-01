#include "MatrixVectorTest.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/MatrixVectorOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/MatrixVectorOperations.h"
#include <string>
#include <sstream>




tarch::la::tests::MatrixVectorTest::MatrixVectorTest():
  TestCase ("tarch::la::MatrixVectorTest") {
}


void tarch::la::tests::MatrixVectorTest::run() {
  testMethod (testMultiplication);
  testMethod (testForwardSubstitution);
  testMethod (testBackSubstitution);
  testMethod (testSolveSystem3x3);
}


void tarch::la::tests::MatrixVectorTest::testMultiplication() {
  Matrix<2,2,int> matrix;
  Vector<2,int> vector;
  matrix = {1, 2, 3, 4};
  vector = {1, 2};

  Vector<2,int> result(0);
  result = multiply(matrix, vector);
  validateEquals (result[0], 5);
  validateEquals (result[1], 11);

}

void tarch::la::tests::MatrixVectorTest::testForwardSubstitution () {
}


void tarch::la::tests::MatrixVectorTest::testBackSubstitution () {
}


void tarch::la::tests::MatrixVectorTest::testSolveSystem3x3() {
//  Matrix<3,3,double> matrix;
//  assignList(matrix) = 1.0, 2.0, 3.0,
//           4.0, 5.0, 6.0,
//           2.0, 3.0, 2.0;
//  Vector<3,double> rhs;
//  rhs = 1.0, 2.0, 3.0;
//  Vector<3,double> x(0.0);
//
//  rhs = solve(matrix, rhs, x);
//  validateNumericalEquals (x(0), -1.166666666666667);
//  validateNumericalEquals (x(1),  2.333333333333333);
//  validateNumericalEquals (x(2), -0.833333333333333);
//
//  assignList(matrix) = 0.0, 1.0, 0.0,
//                       0.0, 0.0, 1.0,
//                       1.0, 1.0, 1.0;
//  rhs = 0.5, 0.5, 1.0;
//  x = 0.0;
//
//  rhs = solveSystem3x3 (matrix, rhs, x);
//  validateNumericalEquals (x(0),  0.0);
//  validateNumericalEquals (x(1),  0.5);
//  validateNumericalEquals (x(2),  0.5);
}
