// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Matrix.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixVectorOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("The product of an integer matrix and an integer vector sums the products along every row") {
  GIVEN("The matrix with the rows 1, 2 and 3, 4 and the vector 1, 2") {
    tarch::la::Matrix<2, 2, int> matrix;
    tarch::la::Vector<2, int>    vector;
    matrix = {1, 2, 3, 4};
    vector = {1, 2};

    WHEN("The matrix multiplies the vector") {
      tarch::la::Vector<2, int> result(0);
      result = tarch::la::multiply(matrix, vector);

      THEN("The product is 5, 11") {
        VALIDATE_EQUALS(result[0], 5);
        VALIDATE_EQUALS(result[1], 11);
      }
    }
  }
}

UNIT_TEST("The product operator and the scaled accumulation apply a matrix to a vector") {
  GIVEN("The matrix with the rows 1, 2 and 3, 4 and the vector 1.5, -0.5") {
    const tarch::la::Matrix<2, 2, double> matrix = {1.0, 2.0, 3.0, 4.0};
    const tarch::la::Vector<2, double>    vector = {1.5, -0.5};

    WHEN("The product operator multiplies the matrix with the vector") {
      const auto result = matrix * vector;

      THEN("The product is 0.5, 2.5") {
        VALIDATE_NUMERICAL_EQUALS(result[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(result[1], 2.5);
      }
    }

    WHEN("The test adds twice the product to a vector of ones") {
      tarch::la::Vector<2, double> accumulator(1.0);
      tarch::la::matVec(matrix, 2.0, vector, accumulator);

      THEN("The sum is 2, 6") {
        VALIDATE_NUMERICAL_EQUALS(accumulator[0], 2.0);
        VALIDATE_NUMERICAL_EQUALS(accumulator[1], 6.0);
      }
    }
  }
}
