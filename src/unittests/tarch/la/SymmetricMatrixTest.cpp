// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/SymmetricMatrix.h"

#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("A symmetric matrix stores its diagonal first and then its upper triangle row by row") {
  GIVEN("A three by three symmetric matrix of the entries 1 to 6 and a two by two one of the entries 1 to 3") {
    tarch::la::SymmetricMatrix<3, double>    matrix3 = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const tarch::la::SymmetricMatrix<2, int> matrix2 = {1, 2, 3};

    THEN("The layout stores the independent entries only and every pair of mirrored entries reads one value") {
      static_assert(tarch::la::SymmetricMatrix<3, double>::NumberOfEntries == 6);
      static_assert(tarch::la::SymmetricMatrix<4, float>::NumberOfEntries == 10);
      static_assert(sizeof(tarch::la::SymmetricMatrix<3, double>) == 6 * sizeof(double));
      static_assert(alignof(tarch::la::SymmetricMatrix<3, float>) == alignof(float));
      static_assert(std::is_trivially_copyable_v<tarch::la::SymmetricMatrix<3, double>>);
      static_assert(std::is_standard_layout_v<tarch::la::SymmetricMatrix<3, double>>);

      VALIDATE_EQUALS(matrix3(0, 0), 1.0);
      VALIDATE_EQUALS(matrix3(1, 1), 2.0);
      VALIDATE_EQUALS(matrix3(2, 2), 3.0);
      VALIDATE_EQUALS(matrix3(0, 1), 4.0);
      VALIDATE_EQUALS(matrix3(0, 2), 5.0);
      VALIDATE_EQUALS(matrix3(1, 2), 6.0);
      VALIDATE_EQUALS(matrix3(1, 0), 4.0);
      VALIDATE_EQUALS(matrix3(2, 0), 5.0);
      VALIDATE_EQUALS(matrix3(2, 1), 6.0);
      VALIDATE_EQUALS(matrix2(0, 0), 1);
      VALIDATE_EQUALS(matrix2(1, 1), 2);
      VALIDATE_EQUALS(matrix2(1, 0), 3);
    }

    WHEN("The test writes 7 into the entry of row 2 and column 1") {
      matrix3(2, 1) = 7.0;

      THEN("The mirrored entry of row 1 and column 2 reads 7 and the stored data contains it in its last entry") {
        VALIDATE_EQUALS(matrix3(1, 2), 7.0);
        VALIDATE_EQUALS(matrix3.getData()[5], 7.0);
      }
    }
  }
}

namespace {
  /**
   * Returns whether every size of matrix stores its diagonal first and then its upper triangle row
   * by row, with both mirrored positions on one entry. The test passes the indices at run time.
   */
  template <int Size>
  bool storesDiagonalFirstAndUpperTriangle(const int runtimeOffset) {
    tarch::la::SymmetricMatrix<Size, int> matrix;
    for (int entry = 0; entry < tarch::la::SymmetricMatrix<Size, int>::NumberOfEntries; entry++) {
      matrix.getData()[entry] = entry;
    }
    int  expectedEntry = Size;
    bool matches       = true;
    for (int rowIndex = 0; rowIndex < Size; rowIndex++) {
      matches = matches and matrix(rowIndex + runtimeOffset, rowIndex + runtimeOffset) == rowIndex;
      for (int columnIndex = rowIndex + 1; columnIndex < Size; columnIndex++) {
        matches = matches and matrix(rowIndex + runtimeOffset, columnIndex + runtimeOffset) == expectedEntry;
        matches = matches and matrix(columnIndex + runtimeOffset, rowIndex + runtimeOffset) == expectedEntry;
        expectedEntry++;
      }
    }
    return matches;
  }
} // namespace

UNIT_TEST("Every size of symmetric matrix reads the entries of the diagonal and of the upper triangle at run time") {
  GIVEN("Symmetric matrices of one to five rows whose stored entries contain their own index") {
    volatile int runtimeOffset = 0;

    THEN("The matrices of one to four rows and the matrix of five rows read every position from its entry") {
      VALIDATE(storesDiagonalFirstAndUpperTriangle<1>(runtimeOffset));
      VALIDATE(storesDiagonalFirstAndUpperTriangle<2>(runtimeOffset));
      VALIDATE(storesDiagonalFirstAndUpperTriangle<3>(runtimeOffset));
      VALIDATE(storesDiagonalFirstAndUpperTriangle<4>(runtimeOffset));
      VALIDATE(storesDiagonalFirstAndUpperTriangle<5>(runtimeOffset));
    }
  }
}

UNIT_TEST("A default symmetric matrix contains zeros") {
  GIVEN("A default three by three symmetric matrix") {
    const tarch::la::SymmetricMatrix<3, double> matrix;

    THEN("Every entry is zero") {
      for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
          VALIDATE_EQUALS(matrix(row, column), 0.0);
        }
      }
    }
  }
}

UNIT_TEST("The arithmetic of symmetric matrices combines the matching entries") {
  GIVEN("The two by two symmetric matrices of the entries 1, 2, 3 and 4, 6, 8") {
    const tarch::la::SymmetricMatrix<2, double> left  = {1.0, 2.0, 3.0};
    const tarch::la::SymmetricMatrix<2, double> right = {4.0, 6.0, 8.0};

    WHEN("The test adds, subtracts and scales the matrices") {
      const tarch::la::SymmetricMatrix<2, double> sum             = left + right;
      const tarch::la::SymmetricMatrix<2, double> difference      = right - left;
      const tarch::la::SymmetricMatrix<2, double> scaledFromRight = left * 2.0;
      const tarch::la::SymmetricMatrix<2, double> scaledFromLeft  = 0.5 * right;

      THEN("The sum is 5, 8, 11, the difference 3, 4, 5 and the scaled matrices 2, 4, 6 and 2, 3, 4") {
        VALIDATE_EQUALS(sum(0, 0), 5.0);
        VALIDATE_EQUALS(sum(1, 1), 8.0);
        VALIDATE_EQUALS(sum(0, 1), 11.0);
        VALIDATE_EQUALS(difference(0, 0), 3.0);
        VALIDATE_EQUALS(difference(1, 1), 4.0);
        VALIDATE_EQUALS(difference(1, 0), 5.0);
        VALIDATE_EQUALS(scaledFromRight(0, 0), 2.0);
        VALIDATE_EQUALS(scaledFromRight(1, 1), 4.0);
        VALIDATE_EQUALS(scaledFromRight(0, 1), 6.0);
        VALIDATE_EQUALS(scaledFromLeft(0, 0), 2.0);
        VALIDATE_EQUALS(scaledFromLeft(1, 1), 3.0);
        VALIDATE_EQUALS(scaledFromLeft(0, 1), 4.0);
      }
    }
  }
}

UNIT_TEST("A symmetric matrix prints its full rows and a vector of symmetric matrices prints every matrix") {
  GIVEN("The symmetric matrix of the entries xx = 1, yy = 2, xy = 3 and a vector of it and its double") {
    const tarch::la::SymmetricMatrix<2, int>                 matrix = {1, 2, 3};
    tarch::la::Vector<2, tarch::la::SymmetricMatrix<2, int>> matrices;
    matrices(0) = matrix;
    matrices(1) = matrix * 2;

    WHEN("The test writes the matrix into a stream and returns the text of the vector") {
      std::ostringstream stream;
      stream << matrix;
      const std::string vectorText = ::toString(matrices);

      THEN("The stream contains the rows 1, 3 and 3, 2 and the vector text contains both matrices") {
        VALIDATE_EQUALS(stream.str(), std::string("[[1,3],[3,2]]"));
        VALIDATE_EQUALS(matrix.toString(), std::string("[[1,3],[3,2]]"));
        VALIDATE_EQUALS(vectorText, std::string("[[[1,3],[3,2]],[[2,6],[6,4]]]"));
        VALIDATE_EQUALS(matrices(1)(1, 0), 6);
      }
    }
  }
}

UNIT_TEST("The product of a symmetric matrix and a vector sums the products along every row") {
  GIVEN("The symmetric matrix of the entries xx = 1, yy = 2, zz = 3, xy = 4, xz = 5, yz = 6 and the vector 1, 2, 3") {
    const tarch::la::SymmetricMatrix<3, double> matrix = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const tarch::la::Vector<3, double>          vector = {1.0, 2.0, 3.0};

    WHEN("The matrix multiplies the vector") {
      const tarch::la::Vector<3, double> product = matrix * vector;

      THEN("The product is 24, 26, 26") {
        VALIDATE_EQUALS(product(0), 24.0);
        VALIDATE_EQUALS(product(1), 26.0);
        VALIDATE_EQUALS(product(2), 26.0);
      }
    }
  }
}
