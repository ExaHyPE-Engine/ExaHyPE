// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Matrix.h"

#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixOperations.h"
#include "tarch/la/MatrixScalarOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("A matrix from one value stores it in every entry and its copy stores it too") {
  GIVEN("A matrix of one row and two columns constructed from the value one") {
    const tarch::la::Matrix<1, 2, int> matrix(1);

    THEN("The matrix stores one in both entries of its one row and two columns") {
      VALIDATE_EQUALS(matrix.getSize(), 2);
      VALIDATE_EQUALS(matrix.getNumberOfRows(), 1);
      VALIDATE_EQUALS(matrix.getNumberOfColumns(), 2);
      VALIDATE_EQUALS(matrix(0, 0), 1);
      VALIDATE_EQUALS(matrix(0, 1), 1);
    }

    WHEN("The test copies the matrix") {
      const tarch::la::Matrix<1, 2, int> copy(matrix);

      THEN("The copy stores one in both entries") {
        VALIDATE_EQUALS(copy(0, 0), 1);
        VALIDATE_EQUALS(copy(0, 1), 1);
      }
    }
  }
}

UNIT_TEST("A matrix initialised from a list of values stores them row by row") {
  GIVEN("A two by two matrix initialised from the list 1, 2, 3, 4 and a one by two matrix from the list 1, 2") {
    const tarch::la::Matrix<2, 2, int> square = {1, 2, 3, 4};
    const tarch::la::Matrix<1, 2, int> row    = {1, 2};

    THEN("Both matrices store the values of their lists row by row") {
      VALIDATE_EQUALS(square(0, 0), 1);
      VALIDATE_EQUALS(square(0, 1), 2);
      VALIDATE_EQUALS(square(1, 0), 3);
      VALIDATE_EQUALS(square(1, 1), 4);
      VALIDATE_EQUALS(row(0, 0), 1);
      VALIDATE_EQUALS(row(0, 1), 2);
    }
  }
}

UNIT_TEST("A matrix prints its rows in brackets and scales with a scalar") {
  GIVEN("The matrix with the rows 1, 2 and 3, 4") {
    tarch::la::Matrix<2, 2, int> matrix = {1, 2, 3, 4};

    WHEN("The test writes the matrix into a stream") {
      std::ostringstream stream;
      stream << matrix;

      THEN("The stream contains the rows in brackets") { VALIDATE_EQUALS(stream.str(), std::string("[[1,2],[3,4]]")); }
    }

    WHEN("The test overwrites the matrix with its product with two") {
      matrix = matrix * 2;

      THEN("Every entry doubles") {
        VALIDATE_EQUALS(matrix(0, 0), 2);
        VALIDATE_EQUALS(matrix(0, 1), 4);
        VALIDATE_EQUALS(matrix(1, 0), 6);
        VALIDATE_EQUALS(matrix(1, 1), 8);
      }
    }
  }
}

UNIT_TEST("The product of two matrices multiplies the rows of the left with the columns of the right matrix") {
  GIVEN("A two by three matrix of 1 to 6 and a three by two matrix of 6 down to 1") {
    const tarch::la::Matrix<2, 3, int> left  = {1, 2, 3, 4, 5, 6};
    const tarch::la::Matrix<3, 2, int> right = {6, 5, 4, 3, 2, 1};

    WHEN("The test multiplies the matrices") {
      const tarch::la::Matrix<2, 2, int> result = left * right;

      THEN("The product has the rows 20, 14 and 56, 41") {
        VALIDATE_EQUALS(result(0, 0), 20);
        VALIDATE_EQUALS(result(0, 1), 14);
        VALIDATE_EQUALS(result(1, 0), 56);
        VALIDATE_EQUALS(result(1, 1), 41);
      }
    }
  }

  GIVEN("A two by five matrix of 1 to 10 and a five by three matrix of 1 to 15") {
    const tarch::la::Matrix<2, 5, int> left  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const tarch::la::Matrix<5, 3, int> right = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    WHEN("The test multiplies the matrices") {
      const tarch::la::Matrix<2, 3, int> product = left * right;

      THEN("The product has the rows 135, 150, 165 and 310, 350, 390") {
        VALIDATE_EQUALS(product(0, 0), 135);
        VALIDATE_EQUALS(product(0, 1), 150);
        VALIDATE_EQUALS(product(0, 2), 165);
        VALIDATE_EQUALS(product(1, 0), 310);
        VALIDATE_EQUALS(product(1, 1), 350);
        VALIDATE_EQUALS(product(1, 2), 390);
      }
    }
  }
}

UNIT_TEST("Two matrices are equal when every entry is equal") {
  GIVEN("Two by three integer matrices of ones and of twos") {
    const tarch::la::Matrix<2, 3, int> ones(1);
    const tarch::la::Matrix<2, 3, int> twos(2);

    THEN("The matrix of ones equals itself and differs from the matrix of twos") {
      VALIDATE(ones == ones);
      VALIDATE(not(ones == twos));
    }
  }

  GIVEN("Two by three double matrices of ones and of twos") {
    const tarch::la::Matrix<2, 3, double> ones(1);
    const tarch::la::Matrix<2, 3, double> twos(2);

    WHEN("The test takes the index of the first entry in which the matrices differ") {
      const std::pair<int, int> index = tarch::la::equalsReturnIndex(ones, twos);

      THEN("The first differing entry lies in row zero and column zero") {
        VALIDATE_EQUALS(index.first, 0);
        VALIDATE_EQUALS(index.second, 0);
      }
    }
  }
}

UNIT_TEST("The sum and the difference of two matrices work entry by entry") {
  GIVEN("A two by five matrix of 1 to 10 and one of 10 down to 1") {
    const tarch::la::Matrix<2, 5, int> ascending  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const tarch::la::Matrix<2, 5, int> descending = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    WHEN("The test adds and subtracts the matrices") {
      const auto sum        = ascending + descending;
      const auto difference = ascending - descending;

      THEN("The sum stores eleven in its first and its last entry and the difference -9 and 9") {
        VALIDATE_EQUALS(sum(0, 0), 11);
        VALIDATE_EQUALS(sum(1, 4), 11);
        VALIDATE_EQUALS(difference(0, 0), -9);
        VALIDATE_EQUALS(difference(1, 4), 9);
      }
    }
  }
}

UNIT_TEST("The transpose swaps rows and columns and a second transpose returns the matrix") {
  GIVEN("A three by two matrix of 1 to 6") {
    const tarch::la::Matrix<3, 2, int> matrix = {1, 2, 3, 4, 5, 6};

    WHEN("The test transposes the matrix once and twice") {
      const tarch::la::Matrix<2, 3, int> transposed      = tarch::la::transpose(matrix);
      const tarch::la::Matrix<3, 2, int> transposedTwice = tarch::la::transpose(transposed);

      THEN("The transpose stores the columns as rows and the second transpose equals the matrix") {
        VALIDATE_EQUALS(transposed(0, 0), 1);
        VALIDATE_EQUALS(transposed(0, 1), 3);
        VALIDATE_EQUALS(transposed(0, 2), 5);
        VALIDATE_EQUALS(transposed(1, 0), 2);
        VALIDATE_EQUALS(transposed(1, 1), 4);
        VALIDATE_EQUALS(transposed(1, 2), 6);
        VALIDATE(transposedTwice == matrix);
      }
    }
  }

  GIVEN("A six by four matrix of 1 to 24") {
    const tarch::la::Matrix<6, 4, int> rectangular = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                      13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

    WHEN("The test transposes the matrix") {
      const auto rectangularTranspose = tarch::la::transpose(rectangular);

      THEN("The last entry of the first row is 21 and the first entry of the last row is four") {
        VALIDATE_EQUALS(rectangularTranspose(0, 5), 21);
        VALIDATE_EQUALS(rectangularTranspose(3, 0), 4);
      }
    }
  }
}

UNIT_TEST("The square matrices of one to four rows store their entries in compact layouts") {
  GIVEN("A two by two matrix of 1 to 4, a three by three matrix of ones and a four by four matrix of twos") {
    const tarch::la::Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
    const tarch::la::Matrix<3, 3, int> matrix3(1);
    const tarch::la::Matrix<4, 4, int> matrix4(2);

    THEN("Every layout is trivially copyable with its size and alignment and every matrix stores its entries") {
      static_assert(sizeof(tarch::la::Matrix<1, 1, int>) == sizeof(int));
      static_assert(std::is_trivially_copyable<tarch::la::Matrix<1, 1, int>>::value);
      static_assert(sizeof(tarch::la::Matrix<2, 2, int>) == 16);
      static_assert(alignof(tarch::la::Matrix<2, 2, int>) == alignof(int));
      static_assert(std::is_standard_layout<tarch::la::Matrix<2, 2, int>>::value);
      static_assert(std::is_trivially_copyable<tarch::la::Matrix<2, 2, int>>::value);
      static_assert(sizeof(tarch::la::Matrix<3, 3, int>) == 36);
      static_assert(alignof(tarch::la::Matrix<3, 3, int>) == alignof(int));
      static_assert(sizeof(tarch::la::Matrix<3, 3, double>) == 72);
      static_assert(std::is_standard_layout<tarch::la::Matrix<3, 3, int>>::value);
      static_assert(std::is_trivially_copyable<tarch::la::Matrix<3, 3, int>>::value);
      static_assert(sizeof(tarch::la::Matrix<4, 4, int>) == 64);
      static_assert(alignof(tarch::la::Matrix<4, 4, int>) == alignof(int));
      static_assert(std::is_standard_layout<tarch::la::Matrix<4, 4, int>>::value);
      static_assert(std::is_trivially_copyable<tarch::la::Matrix<4, 4, int>>::value);

      VALIDATE_EQUALS(matrix2.getNumberOfRows(), 2);
      VALIDATE_EQUALS(matrix2.getNumberOfColumns(), 2);
      VALIDATE_EQUALS(matrix2.getData()[0], 1);
      VALIDATE_EQUALS(matrix2.getData()[3], 4);
      VALIDATE_EQUALS(matrix3.getSize(), 9);
      VALIDATE_EQUALS(matrix3(0, 0), 1);
      VALIDATE_EQUALS(matrix3(2, 2), 1);
      VALIDATE_EQUALS(matrix4.getSize(), 16);
      VALIDATE_EQUALS(matrix4(0, 0), 2);
      VALIDATE_EQUALS(matrix4(3, 3), 2);
    }
  }
}

UNIT_TEST("The conversion of a matrix into another scalar type keeps its shape and its entries") {
  GIVEN("A two by three integer matrix of 1 to 6") {
    const tarch::la::Matrix<2, 3, int> generic = {1, 2, 3, 4, 5, 6};

    WHEN("The matrix converts into doubles") {
      const auto converted = generic.convertScalar<double>();

      THEN("The converted matrix has two rows and three columns, its first entry is one and its last six") {
        VALIDATE_EQUALS(converted.getNumberOfRows(), 2);
        VALIDATE_EQUALS(converted.getNumberOfColumns(), 3);
        VALIDATE_EQUALS(converted(0, 0), 1.0);
        VALIDATE_EQUALS(converted(1, 2), 6.0);
      }
    }
  }

  GIVEN("A three by three integer matrix of 1 to 9 and a four by four integer matrix of ones") {
    const tarch::la::Matrix<3, 3, int> matrix3 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const tarch::la::Matrix<4, 4, int> matrix4 = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    WHEN("Both matrices convert into doubles") {
      const auto converted3 = matrix3.convertScalar<double>();
      const auto converted4 = matrix4.convertScalar<double>();

      THEN("The converted matrices store eight in row two, column one and one in row three, column three") {
        VALIDATE_EQUALS(converted3(2, 1), 8.0);
        VALIDATE_EQUALS(converted4(3, 3), 1.0);
      }
    }
  }
}

UNIT_TEST("A matrix built from an array stores the entries of the array row by row") {
  GIVEN("An array of the integers 1 to 6") {
    const int values[] = {1, 2, 3, 4, 5, 6};

    WHEN("The test builds a two by three matrix from the array") {
      const auto built = tarch::la::buildMatrix<2, 3, int>(values);

      THEN("The first entry of the matrix is one and the last six") {
        VALIDATE_EQUALS(built(0, 0), 1);
        VALIDATE_EQUALS(built(1, 2), 6);
      }
    }
  }
}

UNIT_TEST("The identity matrix stores one on the diagonal and zero beside it") {
  WHEN("The test builds the three by three identity matrix") {
    const tarch::la::Matrix<3, 3, double> identityMatrix = tarch::la::identity<3, double>();

    THEN("The first entry of the diagonal is one and the entry in row one and column two is zero") {
      VALIDATE_EQUALS(identityMatrix(0, 0), 1.0);
      VALIDATE_EQUALS(identityMatrix(1, 2), 0.0);
    }
  }
}

UNIT_TEST("The reductions of a matrix return the sum, the Frobenius norm and the largest magnitude of its entries") {
  GIVEN("A two by three matrix of 1 to 6 and a two by two matrix of -1, 2.5, -7 and 4") {
    const tarch::la::Matrix<2, 3, double> matrix     = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const tarch::la::Matrix<2, 2, double> mixedSigns = {-1.0, 2.5, -7.0, 4.0};

    WHEN("The test takes the sum and the Frobenius norm of the first and the largest magnitude of the second matrix") {
      const double sum              = tarch::la::sum(matrix);
      const double norm             = tarch::la::frobeniusNorm(matrix);
      const double largestMagnitude = tarch::la::elementMax(mixedSigns);

      THEN("The sum is 21, the norm is the root of 91 and the largest magnitude is seven") {
        VALIDATE_EQUALS(sum, 21.0);
        VALIDATE_NUMERICAL_EQUALS(norm, std::sqrt(91.0));
        VALIDATE_EQUALS(largestMagnitude, 7.0);
      }
    }
  }
}

UNIT_TEST("The entry inversion replaces every entry by its reciprocal") {
  GIVEN("A two by two matrix of 1, 2, 4 and 8") {
    const tarch::la::Matrix<2, 2, double> matrix = {1.0, 2.0, 4.0, 8.0};

    WHEN("The test inverts the entries") {
      const tarch::la::Matrix<2, 2, double> inverted = tarch::la::invertEntries(matrix);

      THEN("The matrix stores 1, 0.5, 0.25 and 0.125") {
        VALIDATE_NUMERICAL_EQUALS(inverted(0, 0), 1.0);
        VALIDATE_NUMERICAL_EQUALS(inverted(0, 1), 0.5);
        VALIDATE_NUMERICAL_EQUALS(inverted(1, 0), 0.25);
        VALIDATE_NUMERICAL_EQUALS(inverted(1, 1), 0.125);
      }
    }
  }
}

UNIT_TEST("The diagonal, a row and a column of a matrix contain the entries of the matrix") {
  GIVEN("A three by three matrix of 1 to 9 and a two by three matrix of 1 to 6") {
    const tarch::la::Matrix<3, 3, int>    square = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const tarch::la::Matrix<2, 3, double> matrix = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

    WHEN("The test takes the diagonal of the square matrix and the second row and the third column of the other") {
      const auto diagonal = tarch::la::getDiagonal(square);
      const auto row      = tarch::la::getRow(matrix, 1);
      const auto column   = tarch::la::getColumn(matrix, 2);

      THEN("The diagonal contains 1, 5 and 9, the row runs from four to six and the column contains three and six") {
        VALIDATE_EQUALS(diagonal[0], 1);
        VALIDATE_EQUALS(diagonal[1], 5);
        VALIDATE_EQUALS(diagonal[2], 9);
        VALIDATE_EQUALS(row[0], 4.0);
        VALIDATE_EQUALS(row[2], 6.0);
        VALIDATE_EQUALS(column[0], 3.0);
        VALIDATE_EQUALS(column[1], 6.0);
      }
    }
  }
}

UNIT_TEST("Scaling a square matrix multiplies every entry by the scalar") {
  GIVEN("A two by two integer matrix of 1 to 4") {
    tarch::la::Matrix<2, 2, int> scaled = {1, 2, 3, 4};

    WHEN("The test scales the matrix by three in place") {
      scaled *= 3;

      THEN("The first entry is three and the last is twelve") {
        VALIDATE_EQUALS(scaled(0, 0), 3);
        VALIDATE_EQUALS(scaled(1, 1), 12);
      }

      AND_WHEN("The test overwrites the matrix with the product of two and the matrix") {
        scaled = 2 * scaled;

        THEN("The entry in row zero and column one is twelve and the entry in row one and column zero is 18") {
          VALIDATE_EQUALS(scaled(0, 1), 12);
          VALIDATE_EQUALS(scaled(1, 0), 18);
        }
      }
    }
  }

  GIVEN("A three by three matrix of 1 to 9") {
    tarch::la::Matrix<3, 3, double> scaled = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};

    WHEN("The test scales the matrix by one half in place") {
      scaled *= 0.5;

      THEN("The first entry is one half and the last is 4.5") {
        VALIDATE_NUMERICAL_EQUALS(scaled(0, 0), 0.5);
        VALIDATE_NUMERICAL_EQUALS(scaled(2, 2), 4.5);
      }

      AND_WHEN("The test multiplies the scaled matrix by two and takes the sum and the Frobenius norm of the product") {
        const auto   product       = scaled * 2.0;
        const double sumOfProduct  = tarch::la::sum(product);
        const double normOfProduct = tarch::la::frobeniusNorm(product);

        THEN("The product stores five in its centre, its sum is 45 and its norm is the root of 285") {
          VALIDATE_NUMERICAL_EQUALS(product(1, 1), 5.0);
          VALIDATE_NUMERICAL_EQUALS(sumOfProduct, 45.0);
          VALIDATE_NUMERICAL_EQUALS(normOfProduct, std::sqrt(285.0));
        }
      }
    }
  }
}

UNIT_TEST("The sum, the difference and the component product of two square matrices work entry by entry") {
  GIVEN("Two by two matrices of 1 to 4 and of 4 down to 1") {
    const tarch::la::Matrix<2, 2, int> left  = {1, 2, 3, 4};
    const tarch::la::Matrix<2, 2, int> right = {4, 3, 2, 1};

    THEN("A matrix equals itself") { VALIDATE(tarch::la::equals(left, left)); }

    WHEN("The test adds, subtracts and multiplies the matrices component by component") {
      const auto sum              = left + right;
      const auto difference       = left - right;
      const auto componentProduct = tarch::la::multiplyComponents(left, right);

      THEN("The sum starts with 5, the difference ends with 3 and the component product has 4 on its diagonal") {
        VALIDATE_EQUALS(sum(0, 0), 5);
        VALIDATE_EQUALS(difference(1, 1), 3);
        VALIDATE_EQUALS(componentProduct(0, 0), 4);
        VALIDATE_EQUALS(componentProduct(1, 1), 4);
      }
    }
  }

  GIVEN("Three by three matrices of 1 to 9 and of 9 down to 1") {
    const tarch::la::Matrix<3, 3, int> left  = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const tarch::la::Matrix<3, 3, int> right = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    THEN("A matrix equals itself and differs from the other matrix") {
      VALIDATE(tarch::la::equals(left, left));
      VALIDATE(not(left == right));
    }

    WHEN("The test adds, subtracts and multiplies the matrices component by component") {
      const auto sum              = left + right;
      const auto difference       = left - right;
      const auto componentProduct = tarch::la::multiplyComponents(left, right);

      THEN("The sum is 10 at both ends, the difference -8 and 8 and the component product 25 in its centre") {
        VALIDATE_EQUALS(sum(0, 0), 10);
        VALIDATE_EQUALS(sum(2, 2), 10);
        VALIDATE_EQUALS(difference(0, 0), -8);
        VALIDATE_EQUALS(difference(2, 2), 8);
        VALIDATE_EQUALS(componentProduct(1, 1), 25);
      }
    }
  }
}

UNIT_TEST("A four by four matrix returns its diagonal, its rows, its columns, its transpose and its reductions") {
  GIVEN("A four by four matrix of 1 to 16 and one of 1 to 16 with alternating signs") {
    const tarch::la::Matrix<4, 4, int> matrix      = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    const tarch::la::Matrix<4, 4, int> alternating = {-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16};

    WHEN("The test takes the diagonal, the third row, the second column and the transpose of the first matrix") {
      const auto diagonal   = tarch::la::getDiagonal(matrix);
      const auto row        = tarch::la::getRow(matrix, 2);
      const auto column     = tarch::la::getColumn(matrix, 1);
      const auto transposed = tarch::la::transpose(matrix);

      THEN("The diagonal, the row, the column and the transpose contain the entries of the matrix at their positions") {
        VALIDATE_EQUALS(diagonal[0], 1);
        VALIDATE_EQUALS(diagonal[1], 6);
        VALIDATE_EQUALS(diagonal[2], 11);
        VALIDATE_EQUALS(diagonal[3], 16);
        VALIDATE_EQUALS(row[0], 9);
        VALIDATE_EQUALS(row[3], 12);
        VALIDATE_EQUALS(column[0], 2);
        VALIDATE_EQUALS(column[3], 14);
        VALIDATE_EQUALS(transposed(0, 3), 13);
        VALIDATE_EQUALS(transposed(3, 0), 4);
      }
    }

    WHEN("The test takes the sum of the first matrix and the largest magnitude of the second matrix") {
      const int sum              = tarch::la::sum(matrix);
      const int largestMagnitude = tarch::la::elementMax(alternating);

      THEN("The sum is 136 and the largest magnitude is 16") {
        VALIDATE_EQUALS(sum, 136);
        VALIDATE_EQUALS(largestMagnitude, 16);
      }
    }
  }
}

UNIT_TEST("A vector of matrices accesses the entries of every matrix and prints every matrix in brackets") {
  GIVEN("A vector of the two matrices with the rows 1, 2 and 3, 4 and with the rows 5, 6 and 7, 8") {
    tarch::la::Vector<2, tarch::la::Matrix<2, 2, int>> matrices;
    matrices(0) = {1, 2, 3, 4};
    matrices(1) = {5, 6, 7, 8};

    WHEN("The test overwrites the entry of row 1 and column 0 of the second matrix with 9") {
      matrices(1)(1, 0) = 9;

      THEN("The second matrix reads 9 there and the text of the vector contains both matrices") {
        VALIDATE_EQUALS(matrices(1)(1, 0), 9);
        VALIDATE_EQUALS(matrices(0)(1, 0), 3);
        VALIDATE_EQUALS(::toString(matrices), std::string("[[[1,2],[3,4]],[[5,6],[9,8]]]"));
      }
    }
  }
}

UNIT_TEST("Every square matrix of two to four rows prints itself") {
  GIVEN("Square matrices of two, three and four rows") {
    const tarch::la::Matrix<2, 2, int> matrix2 = {1, 2, 3, 4};
    const tarch::la::Matrix<3, 3, int> matrix3 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const tarch::la::Matrix<4, 4, int> matrix4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    WHEN("The matrices return their texts") {
      const std::string prettyText2 = matrix2.toPrettyString();
      const std::string text2       = matrix2.toString();
      const std::string prettyText3 = matrix3.toPrettyString();
      const std::string prettyText4 = matrix4.toPrettyString();

      THEN("Every text contains characters") {
        VALIDATE(not prettyText2.empty());
        VALIDATE(not text2.empty());
        VALIDATE(not prettyText3.empty());
        VALIDATE(not prettyText4.empty());
      }
    }
  }
}

UNIT_TEST("A generic five by five matrix supports the operations of the square matrices of two to four rows") {
  GIVEN("A five by five matrix of 1 to 25") {
    const tarch::la::Matrix<5, 5, double> generic5 = {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,
                                                      10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0,
                                                      19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0};

    WHEN("The test takes the sum, the Frobenius norm and the largest magnitude of the matrix") {
      const double sum              = tarch::la::sum(generic5);
      const double norm             = tarch::la::frobeniusNorm(generic5);
      const double largestMagnitude = tarch::la::elementMax(generic5);

      THEN("The sum is 325, the norm is the root of 5525 and the largest magnitude is 25") {
        VALIDATE_EQUALS(sum, 325.0);
        VALIDATE_NUMERICAL_EQUALS(norm, std::sqrt(5525.0));
        VALIDATE_EQUALS(largestMagnitude, 25.0);
      }
    }

    WHEN("The test takes the transpose, the diagonal, the fourth row and the second column of the matrix") {
      const auto transposed = tarch::la::transpose(generic5);
      const auto diagonal   = tarch::la::getDiagonal(generic5);
      const auto row        = tarch::la::getRow(generic5, 3);
      const auto column     = tarch::la::getColumn(generic5, 1);

      THEN("The transpose, the diagonal, the row and the column contain the entries of the matrix at their positions") {
        VALIDATE_EQUALS(transposed(0, 4), 21.0);
        VALIDATE_EQUALS(transposed(4, 0), 5.0);
        VALIDATE_EQUALS(diagonal[0], 1.0);
        VALIDATE_EQUALS(diagonal[2], 13.0);
        VALIDATE_EQUALS(diagonal[4], 25.0);
        VALIDATE_EQUALS(row[0], 16.0);
        VALIDATE_EQUALS(row[4], 20.0);
        VALIDATE_EQUALS(column[0], 2.0);
        VALIDATE_EQUALS(column[4], 22.0);
      }
    }

    WHEN("The test multiplies the matrix by one half") {
      const auto scaled = generic5 * 0.5;

      THEN("The first entry is one half and the last is 12.5") {
        VALIDATE_EQUALS(scaled(0, 0), 0.5);
        VALIDATE_EQUALS(scaled(4, 4), 12.5);
      }
    }
  }

  GIVEN("A five by five matrix of 25 divisors of 100000 from 1 to 100000") {
    const tarch::la::Matrix<5, 5, double> divisors = {1.0,     2.0,     4.0,     5.0,     10.0,   20.0,    25.0,
                                                      50.0,    100.0,   125.0,   200.0,   250.0,  400.0,   500.0,
                                                      1000.0,  2000.0,  2500.0,  4000.0,  5000.0, 10000.0, 20000.0,
                                                      25000.0, 40000.0, 50000.0, 100000.0};

    WHEN("The test inverts the entries") {
      const tarch::la::Matrix<5, 5, double> inverted = tarch::la::invertEntries(divisors);

      THEN("The first two entries are one and one half and the last is 0.00001") {
        VALIDATE_NUMERICAL_EQUALS(inverted(0, 0), 1.0);
        VALIDATE_NUMERICAL_EQUALS(inverted(0, 1), 0.5);
        VALIDATE_NUMERICAL_EQUALS(inverted(4, 4), 0.00001);
      }
    }
  }
}

#if not defined(PEANO_WITH_FPE)
UNIT_TEST("A NaN entry makes every tolerance comparison of matrices report a difference, whatever the tolerance") {
  GIVEN("The tolerance of the largest double, a finite matrix, a copy with a NaN and square matrices with a NaN") {
    constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();
    constexpr double Tolerance  = std::numeric_limits<double>::max();

    const tarch::la::Matrix<2, 3, double> finite  = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const tarch::la::Matrix<2, 3, double> withNaN = {1.0, 2.0, 3.0, 4.0, NotANumber, 6.0};
    const tarch::la::Matrix<2, 2, double> square2 = {1.0, NotANumber, 3.0, 4.0};
    const tarch::la::Matrix<3, 3, double> square3 = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, NotANumber};

    THEN("Every comparison that involves a NaN reports a difference and the finite matrix equals itself") {
      VALIDATE(tarch::la::equals(finite, finite, Tolerance));
      VALIDATE(not tarch::la::equals(finite, withNaN, Tolerance));
      VALIDATE(not tarch::la::equals(square2, square2, Tolerance));
      VALIDATE(not tarch::la::equals(square3, square3, Tolerance));
    }

    WHEN("The test takes the index of the first entry in which the finite matrix and its copy with a NaN differ") {
      const std::pair<int, int> index = tarch::la::equalsReturnIndex(finite, withNaN, Tolerance);

      THEN("The index is the position of the NaN") {
        VALIDATE_EQUALS(index.first, 1);
        VALIDATE_EQUALS(index.second, 1);
      }
    }
  }
}
#endif

UNIT_TEST("The component product of two matrices of the same shape multiplies the entries at equal positions") {
  GIVEN("A two by three matrix of 1 to 6 and one of 6 down to 1") {
    const tarch::la::Matrix<2, 3, int> left  = {1, 2, 3, 4, 5, 6};
    const tarch::la::Matrix<2, 3, int> right = {6, 5, 4, 3, 2, 1};

    WHEN("The test multiplies the matrices component by component") {
      const tarch::la::Matrix<2, 3, int> product = tarch::la::multiplyComponents(left, right);

      THEN("The product has the rows 6, 10, 12 and 12, 10, 6") {
        VALIDATE_EQUALS(product(0, 0), 6);
        VALIDATE_EQUALS(product(0, 1), 10);
        VALIDATE_EQUALS(product(0, 2), 12);
        VALIDATE_EQUALS(product(1, 0), 12);
        VALIDATE_EQUALS(product(1, 1), 10);
        VALIDATE_EQUALS(product(1, 2), 6);
      }
    }
  }
}
