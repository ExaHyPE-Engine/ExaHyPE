// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Vector.h"

#include "tarch/la/VectorCompare.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/testing/UnitTest.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include <sycl/sycl.hpp>
#endif

UNIT_TEST("A vector stores the value, the list or the vector that its constructor receives") {
  GIVEN("Two vectors constructed from the value one and two vectors initialised from the lists 1, 2 and 1, 2, 3") {
    const tarch::la::Vector<1, int> oneEntry(1);
    const tarch::la::Vector<2, int> twoEntries(1);
    const tarch::la::Vector<2, int> twoEntriesFromList   = {1, 2};
    const tarch::la::Vector<3, int> threeEntriesFromList = {1, 2, 3};

    THEN("The vectors from the value one store one in every entry and the other vectors store their lists") {
      VALIDATE_EQUALS(oneEntry[0], 1);
      VALIDATE_EQUALS(twoEntries[0], 1);
      VALIDATE_EQUALS(twoEntries[1], 1);
      VALIDATE_EQUALS(twoEntriesFromList[0], 1);
      VALIDATE_EQUALS(twoEntriesFromList[1], 2);
      VALIDATE_EQUALS(threeEntriesFromList[0], 1);
      VALIDATE_EQUALS(threeEntriesFromList[1], 2);
      VALIDATE_EQUALS(threeEntriesFromList[2], 3);
    }

    WHEN("The test copies the vector of the list 1, 2, 3 by construction and the copy by initialisation") {
      const tarch::la::Vector<3, int> constructedCopy(threeEntriesFromList);
      const tarch::la::Vector<3, int> initialisedCopy = constructedCopy;

      THEN("Both copies store 1, 2, 3") {
        VALIDATE_EQUALS(constructedCopy[0], 1);
        VALIDATE_EQUALS(constructedCopy[1], 2);
        VALIDATE_EQUALS(constructedCopy[2], 3);
        VALIDATE_EQUALS(initialisedCopy[0], 1);
        VALIDATE_EQUALS(initialisedCopy[1], 2);
        VALIDATE_EQUALS(initialisedCopy[2], 3);
      }
    }
  }
}

UNIT_TEST("A vector of every size built from a brace list is a constant expression that stores the list") {
  GIVEN("Constant vectors of one to five entries built from brace lists") {
    constexpr tarch::la::Vector<1, double> Vector1{1.5};
    constexpr tarch::la::Vector<2, double> Vector2{1.5, 2.5};
    constexpr tarch::la::Vector<3, int>    Vector3{1, 2, 3};
    constexpr tarch::la::Vector<4, bool>   Vector4{true, false, true, false};
    constexpr tarch::la::Vector<5, double> Vector5{1.5, 2.5, 3.5, 4.5, 5.5};

    THEN("Every vector stores its list") {
      VALIDATE_NUMERICAL_EQUALS(Vector1[0], 1.5);
      for (int entry = 0; entry < 2; entry++) {
        VALIDATE_NUMERICAL_EQUALS(Vector2[entry], entry + 1.5, entry);
      }
      for (int entry = 0; entry < 3; entry++) {
        VALIDATE_EQUALS(Vector3[entry], entry + 1, entry);
      }
      for (int entry = 0; entry < 4; entry++) {
        const bool entryIsEven = entry % 2 == 0;
        VALIDATE_EQUALS(Vector4[entry], entryIsEven, entry);
      }
      for (int entry = 0; entry < 5; entry++) {
        VALIDATE_NUMERICAL_EQUALS(Vector5[entry], entry + 1.5, entry);
      }
    }
  }
}

UNIT_TEST("A default-constructed vector of every size is a constant expression that stores zeros") {
  GIVEN("Default-constructed constant vectors of one to five entries") {
    constexpr tarch::la::Vector<1, int>    Vector1{};
    constexpr tarch::la::Vector<2, int>    Vector2{};
    constexpr tarch::la::Vector<3, double> Vector3{};
    constexpr tarch::la::Vector<4, int>    Vector4{};
    constexpr tarch::la::Vector<5, double> Vector5{};

    THEN("Every entry of every vector is zero") {
      VALIDATE_EQUALS(Vector1[0], 0);
      for (int entry = 0; entry < 2; entry++) {
        VALIDATE_EQUALS(Vector2[entry], 0, entry);
      }
      for (int entry = 0; entry < 3; entry++) {
        VALIDATE_EQUALS(Vector3[entry], 0.0, entry);
      }
      for (int entry = 0; entry < 4; entry++) {
        VALIDATE_EQUALS(Vector4[entry], 0, entry);
      }
      for (int entry = 0; entry < 5; entry++) {
        VALIDATE_EQUALS(Vector5[entry], 0.0, entry);
      }
    }
  }
}

UNIT_TEST("A vector stores the list that initialises it or that an assignment writes into it") {
  GIVEN("A vector initialised from the list 1, 2, 3") {
    tarch::la::Vector<3, int> vector = {1, 2, 3};

    THEN("The vector stores 1, 2, 3") {
      VALIDATE_EQUALS(vector[0], 1);
      VALIDATE_EQUALS(vector[1], 2);
      VALIDATE_EQUALS(vector[2], 3);
    }

    WHEN("The test assigns the list 1, 2, 3 to the vector") {
      vector = {1, 2, 3};

      THEN("The vector stores 1, 2, 3") {
        VALIDATE_EQUALS(vector[0], 1, toString(vector));
        VALIDATE_EQUALS(vector[1], 2, toString(vector));
        VALIDATE_EQUALS(vector[2], 3, toString(vector));
      }
    }
  }
}

UNIT_TEST("The reductions of a vector of three integers return its sum, its norm, its extremes and their indices") {
  GIVEN("The vectors 1, -2, 2 and 1, 2, 3") {
    const tarch::la::Vector<3, int> mixedSigns = {1, -2, 2};
    const tarch::la::Vector<3, int> ascending  = {1, 2, 3};

    WHEN("The test takes the sum and the 1-norm of the first vector and the extremes of the second vector") {
      const int sum             = tarch::la::sum(mixedSigns);
      const int sumOfMagnitudes = tarch::la::norm1(mixedSigns);
      const int indexOfMinimum  = tarch::la::indexMin(ascending);
      const int indexOfMaximum  = tarch::la::indexMax(ascending);
      const int minimum         = tarch::la::min(ascending);
      const int maximum         = tarch::la::max(ascending);

      THEN("The sum is one and the norm five and the minimum one lies at index zero and the maximum three at two") {
        VALIDATE_EQUALS(sum, 1);
        VALIDATE_EQUALS(sumOfMagnitudes, 5.0);
        VALIDATE_EQUALS(indexOfMinimum, 0);
        VALIDATE_EQUALS(indexOfMaximum, 2);
        VALIDATE_EQUALS(minimum, 1);
        VALIDATE_EQUALS(maximum, 3);
      }
    }
  }
}

UNIT_TEST("A vector prints its entries in brackets") {
  GIVEN("The vector 1, 2, 3") {
    const tarch::la::Vector<3, int> vector = {1, 2, 3};

    WHEN("The test writes the vector into a stream") {
      std::ostringstream stream;
      stream << vector;
      const std::string text(stream.str());

      THEN("The stream contains the entries in brackets") { VALIDATE_EQUALS(text, std::string("[1,2,3]")); }
    }
  }
}

UNIT_TEST("The reductions and the predicates of a vector of two entries cover every entry") {
  GIVEN("The vectors -1.5, 3 and 4, 4") {
    const tarch::la::Vector<2, double> mixedSigns = {-1.5, 3.0};
    const tarch::la::Vector<2, int>    fours      = {4, 4};

    THEN("Both entries of the second vector are the same") { VALIDATE(tarch::la::allEntriesAreTheSame(fours)); }

    WHEN("The test takes the squared 2-norm and the maximum norm of the first vector and the fours in the second") {
      const double squaredNorm   = tarch::la::norm2Squared(mixedSigns);
      const double maximumNorm   = tarch::la::normMax(mixedSigns);
      const int    numberOfFours = tarch::la::count(fours, 4);

      THEN("The squared norm is 11.25, the maximum norm three and the second vector stores two fours") {
        VALIDATE_NUMERICAL_EQUALS(squaredNorm, 11.25);
        VALIDATE_NUMERICAL_EQUALS(maximumNorm, 3.0);
        VALIDATE_EQUALS(numberOfFours, 2);
      }
    }
  }
}

UNIT_TEST("The reductions and the predicates of a vector of four entries cover every entry") {
  GIVEN("The vectors 3, -1, 5, 2 and 2, 3, 4, 5 and a vector of the finite values 1 to 4") {
    const tarch::la::Vector<4, int>    mixedSigns = {3, -1, 5, 2};
    const tarch::la::Vector<4, int>    ascending  = {2, 3, 4, 5};
    const tarch::la::Vector<4, double> finite     = {1.0, 2.0, 3.0, 4.0};

    THEN("The first vector contains two but not seven and every entry of the last vector is finite") {
      VALIDATE(tarch::la::contains(mixedSigns, 2));
      VALIDATE(not tarch::la::contains(mixedSigns, 7));
      VALIDATE(tarch::la::isEntryFinite(finite));
    }

    WHEN("The test takes the sum and the extremes of the first vector and the volume of the second vector") {
      const int sum              = tarch::la::sum(mixedSigns);
      const int volume           = tarch::la::volume(ascending);
      const int indexOfMinimum   = tarch::la::indexMin(mixedSigns);
      const int indexOfMaximum   = tarch::la::indexMax(mixedSigns);
      const int minimum          = tarch::la::min(mixedSigns);
      const int maximum          = tarch::la::max(mixedSigns);
      const int largestMagnitude = tarch::la::maxAbs(mixedSigns);

      THEN("The sum is nine and the volume of the second vector is 120") {
        VALIDATE_EQUALS(sum, 9);
        VALIDATE_EQUALS(volume, 120);
      }

      THEN("The minimum -1 lies at index one, the maximum five at index two and the largest magnitude is five") {
        VALIDATE_EQUALS(indexOfMinimum, 1);
        VALIDATE_EQUALS(indexOfMaximum, 2);
        VALIDATE_EQUALS(minimum, -1);
        VALIDATE_EQUALS(maximum, 5);
        VALIDATE_EQUALS(largestMagnitude, 5);
      }
    }
  }
}

UNIT_TEST("The floor of a vector rounds every entry down") {
  GIVEN("The vector 1.2, -1.2, 3.99, 4.01") {
    const tarch::la::Vector<4, double> vector = {1.2, -1.2, 3.99, 4.01};

    WHEN("The test takes the floor of the vector") {
      const tarch::la::Vector<4, double> floorResult = tarch::la::floor(vector);

      THEN("The floor is 1, -2, 3, 4") {
        VALIDATE_NUMERICAL_EQUALS(floorResult[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(floorResult[1], -2.0);
        VALIDATE_NUMERICAL_EQUALS(floorResult[2], 3.0);
        VALIDATE_NUMERICAL_EQUALS(floorResult[3], 4.0);
      }
    }
  }

  GIVEN("The generic vector 1.2, 2.9, -1.2, -3, 0, 4.01, 5.99") {
    const tarch::la::Vector<7, double> generic7 = {1.2, 2.9, -1.2, -3.0, 0.0, 4.01, 5.99};

    WHEN("The test takes the floor of the vector") {
      const tarch::la::Vector<7, double> generic7Floor = tarch::la::floor(generic7);

      THEN("The floor is 1, 2, -2, -3, 0, 4, 5") {
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[1], 2.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[2], -2.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[3], -3.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[4], 0.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[5], 4.0);
        VALIDATE_NUMERICAL_EQUALS(generic7Floor[6], 5.0);
      }
    }
  }
}

UNIT_TEST("The entry inversion of a vector replaces every entry by its reciprocal") {
  GIVEN("The vector 2, 4, 8") {
    const tarch::la::Vector<3, double> vector = {2.0, 4.0, 8.0};

    WHEN("The test inverts the entries") {
      const tarch::la::Vector<3, double> inverted = tarch::la::invertEntries(vector);

      THEN("The vector stores 0.5, 0.25, 0.125") {
        VALIDATE_NUMERICAL_EQUALS(inverted[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(inverted[1], 0.25);
        VALIDATE_NUMERICAL_EQUALS(inverted[2], 0.125);
      }
    }
  }

  GIVEN("The generic vector 2, 4, 5, 8, 10") {
    const tarch::la::Vector<5, double> generic5 = {2.0, 4.0, 5.0, 8.0, 10.0};

    WHEN("The test inverts the entries") {
      const tarch::la::Vector<5, double> generic5Inverted = tarch::la::invertEntries(generic5);

      THEN("The vector stores 0.5, 0.25, 0.2, 0.125, 0.1") {
        VALIDATE_NUMERICAL_EQUALS(generic5Inverted[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(generic5Inverted[1], 0.25);
        VALIDATE_NUMERICAL_EQUALS(generic5Inverted[2], 0.2);
        VALIDATE_NUMERICAL_EQUALS(generic5Inverted[3], 0.125);
        VALIDATE_NUMERICAL_EQUALS(generic5Inverted[4], 0.1);
      }
    }
  }
}

UNIT_TEST("The reductions and the predicates of a generic vector of five entries cover every entry") {
  GIVEN("The generic vectors 1, -2, 3, -4, 5 and 1, 2, 1, 3, 1") {
    const tarch::la::Vector<5, int> generic5 = {1, -2, 3, -4, 5};
    const tarch::la::Vector<5, int> withOnes = {1, 2, 1, 3, 1};

    THEN("The first vector contains three but not nine") {
      VALIDATE(tarch::la::contains(generic5, 3));
      VALIDATE(not tarch::la::contains(generic5, 9));
    }

    WHEN("The test takes the sum, the norms and the extremes of the first vector and the ones in the second vector") {
      const int sum              = tarch::la::sum(generic5);
      const int sumOfMagnitudes  = tarch::la::norm1(generic5);
      const int squaredNorm      = tarch::la::norm2Squared(generic5);
      const int maximumNorm      = tarch::la::normMax(generic5);
      const int indexOfMinimum   = tarch::la::indexMin(generic5);
      const int indexOfMaximum   = tarch::la::indexMax(generic5);
      const int minimum          = tarch::la::min(generic5);
      const int maximum          = tarch::la::max(generic5);
      const int largestMagnitude = tarch::la::maxAbs(generic5);
      const int numberOfOnes     = tarch::la::count(withOnes, 1);

      THEN("The sum is 3, the 1-norm 15, the squared 2-norm 55 and the maximum norm and the largest magnitude 5") {
        VALIDATE_EQUALS(sum, 3);
        VALIDATE_EQUALS(sumOfMagnitudes, 15);
        VALIDATE_EQUALS(squaredNorm, 55);
        VALIDATE_EQUALS(maximumNorm, 5);
        VALIDATE_EQUALS(largestMagnitude, 5);
      }

      THEN("The minimum -4 lies at index 3, the maximum 5 at index 4 and the second vector stores three ones") {
        VALIDATE_EQUALS(indexOfMinimum, 3);
        VALIDATE_EQUALS(indexOfMaximum, 4);
        VALIDATE_EQUALS(minimum, -4);
        VALIDATE_EQUALS(maximum, 5);
        VALIDATE_EQUALS(numberOfOnes, 3);
      }
    }
  }
}

UNIT_TEST("The operations of a vector with a scalar apply the scalar to every entry") {
  GIVEN("A vector of three integer zeros") {
    tarch::la::Vector<3, int> vector(0);

    WHEN("The vector adds one, subtracts two, multiplies by -2 and divides by two in place") {
      vector += 1;
      const tarch::la::Vector<3, int> afterTheAddition = vector;
      vector -= 2;
      const tarch::la::Vector<3, int> afterTheSubtraction = vector;
      vector *= -2;
      const tarch::la::Vector<3, int> afterTheMultiplication = vector;
      vector /= 2;
      const tarch::la::Vector<3, int> afterTheDivision = vector;

      THEN("Every entry is one, -1, two and one after the four steps") {
        VALIDATE_EQUALS(afterTheAddition[0], 1);
        VALIDATE_EQUALS(afterTheAddition[1], 1);
        VALIDATE_EQUALS(afterTheAddition[2], 1);
        VALIDATE_EQUALS(afterTheSubtraction[0], -1);
        VALIDATE_EQUALS(afterTheSubtraction[1], -1);
        VALIDATE_EQUALS(afterTheSubtraction[2], -1);
        VALIDATE_EQUALS(afterTheMultiplication[0], 2);
        VALIDATE_EQUALS(afterTheMultiplication[1], 2);
        VALIDATE_EQUALS(afterTheMultiplication[2], 2);
        VALIDATE_EQUALS(afterTheDivision[0], 1);
        VALIDATE_EQUALS(afterTheDivision[1], 1);
        VALIDATE_EQUALS(afterTheDivision[2], 1);
      }

      AND_WHEN("The vector takes its product with two, two times it, its quotient by four, plus one and minus one") {
        vector                                               = vector * 2;
        const tarch::la::Vector<3, int> afterTheRightProduct = vector;
        vector                                               = 2 * vector;
        const tarch::la::Vector<3, int> afterTheLeftProduct  = vector;
        vector                                               = vector / 4;
        const tarch::la::Vector<3, int> afterTheQuotient     = vector;
        vector                                               = vector + 1;
        const tarch::la::Vector<3, int> afterTheSum          = vector;
        vector                                               = vector - 1;

        THEN("Every entry is two, four, one, two and one after the five steps") {
          VALIDATE_EQUALS(afterTheRightProduct[0], 2);
          VALIDATE_EQUALS(afterTheRightProduct[1], 2);
          VALIDATE_EQUALS(afterTheRightProduct[2], 2);
          VALIDATE_EQUALS(afterTheLeftProduct[0], 4);
          VALIDATE_EQUALS(afterTheLeftProduct[1], 4);
          VALIDATE_EQUALS(afterTheLeftProduct[2], 4);
          VALIDATE_EQUALS(afterTheQuotient[0], 1);
          VALIDATE_EQUALS(afterTheQuotient[1], 1);
          VALIDATE_EQUALS(afterTheQuotient[2], 1);
          VALIDATE_EQUALS(afterTheSum[0], 2);
          VALIDATE_EQUALS(afterTheSum[1], 2);
          VALIDATE_EQUALS(afterTheSum[2], 2);
          VALIDATE_EQUALS(vector[0], 1, toString(vector));
          VALIDATE_EQUALS(vector[1], 1, toString(vector));
          VALIDATE_EQUALS(vector[2], 1, toString(vector));
        }
      }
    }
  }

  GIVEN("The vector 1, 2 of doubles") {
    tarch::la::Vector<2, double> vector2 = {1.0, 2.0};

    WHEN("The vector adds 0.5, subtracts one, multiplies by four and divides by two in place") {
      vector2 += 0.5;
      const tarch::la::Vector<2, double> afterTheAddition = vector2;
      vector2 -= 1.0;
      const tarch::la::Vector<2, double> afterTheSubtraction = vector2;
      vector2 *= 4.0;
      const tarch::la::Vector<2, double> afterTheMultiplication = vector2;
      vector2 /= 2.0;
      const tarch::la::Vector<2, double> afterTheDivision = vector2;

      THEN("The vector is 1.5, 2.5 and 0.5, 1.5 and 2, 6 and 1, 3 after the four steps") {
        VALIDATE_NUMERICAL_EQUALS(afterTheAddition[0], 1.5);
        VALIDATE_NUMERICAL_EQUALS(afterTheAddition[1], 2.5);
        VALIDATE_NUMERICAL_EQUALS(afterTheSubtraction[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(afterTheSubtraction[1], 1.5);
        VALIDATE_NUMERICAL_EQUALS(afterTheMultiplication[0], 2.0);
        VALIDATE_NUMERICAL_EQUALS(afterTheMultiplication[1], 6.0);
        VALIDATE_NUMERICAL_EQUALS(afterTheDivision[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(afterTheDivision[1], 3.0);
      }

      AND_WHEN("The vector takes its sum with two, its difference with one, one half of it and its quotient by two") {
        vector2                                               = vector2 + 2.0;
        const tarch::la::Vector<2, double> afterTheSum        = vector2;
        vector2                                               = vector2 - 1.0;
        const tarch::la::Vector<2, double> afterTheDifference = vector2;
        vector2                                               = 0.5 * vector2;
        const tarch::la::Vector<2, double> afterTheProduct    = vector2;
        vector2                                               = vector2 / 2.0;

        THEN("The vector is 3, 5 and 2, 4 and 1, 2 and 0.5, 1 after the four steps") {
          VALIDATE_NUMERICAL_EQUALS(afterTheSum[0], 3.0);
          VALIDATE_NUMERICAL_EQUALS(afterTheSum[1], 5.0);
          VALIDATE_NUMERICAL_EQUALS(afterTheDifference[0], 2.0);
          VALIDATE_NUMERICAL_EQUALS(afterTheDifference[1], 4.0);
          VALIDATE_NUMERICAL_EQUALS(afterTheProduct[0], 1.0);
          VALIDATE_NUMERICAL_EQUALS(afterTheProduct[1], 2.0);
          VALIDATE_NUMERICAL_EQUALS(vector2[0], 0.5);
          VALIDATE_NUMERICAL_EQUALS(vector2[1], 1.0);
        }
      }
    }
  }

  GIVEN("The generic vector 1, 2, 3, 4, 5") {
    tarch::la::Vector<5, int> generic5 = {1, 2, 3, 4, 5};

    WHEN("The vector adds two, subtracts one, multiplies by three and divides by three in place") {
      generic5 += 2;
      const tarch::la::Vector<5, int> afterTheAddition = generic5;
      generic5 -= 1;
      const tarch::la::Vector<5, int> afterTheSubtraction = generic5;
      generic5 *= 3;
      const tarch::la::Vector<5, int> afterTheMultiplication = generic5;
      generic5 /= 3;
      const tarch::la::Vector<5, int> afterTheDivision = generic5;

      THEN("The first and the last entry are 3, 7 and 2, 6 and 6, 18 and 2, 6 after the four steps") {
        VALIDATE_EQUALS(afterTheAddition[0], 3);
        VALIDATE_EQUALS(afterTheAddition[4], 7);
        VALIDATE_EQUALS(afterTheSubtraction[0], 2);
        VALIDATE_EQUALS(afterTheSubtraction[4], 6);
        VALIDATE_EQUALS(afterTheMultiplication[0], 6);
        VALIDATE_EQUALS(afterTheMultiplication[4], 18);
        VALIDATE_EQUALS(afterTheDivision[0], 2);
        VALIDATE_EQUALS(afterTheDivision[4], 6);
      }

      AND_WHEN("The vector takes its sum with one, its difference with two, two times it and its quotient by two") {
        generic5                                           = generic5 + 1;
        const tarch::la::Vector<5, int> afterTheSum        = generic5;
        generic5                                           = generic5 - 2;
        const tarch::la::Vector<5, int> afterTheDifference = generic5;
        generic5                                           = 2 * generic5;
        const tarch::la::Vector<5, int> afterTheProduct    = generic5;
        generic5                                           = generic5 / 2;

        THEN("The first and the last entry are 3, 7 and 1, 5 and 2, 10 and 1, 5 after the four steps") {
          VALIDATE_EQUALS(afterTheSum[0], 3);
          VALIDATE_EQUALS(afterTheSum[4], 7);
          VALIDATE_EQUALS(afterTheDifference[0], 1);
          VALIDATE_EQUALS(afterTheDifference[4], 5);
          VALIDATE_EQUALS(afterTheProduct[0], 2);
          VALIDATE_EQUALS(afterTheProduct[4], 10);
          VALIDATE_EQUALS(generic5[0], 1);
          VALIDATE_EQUALS(generic5[4], 5);
        }
      }
    }
  }
}

UNIT_TEST("The comparisons of a vector with a scalar test one entry or every entry") {
  GIVEN("The vector 2, 2, 3, 4") {
    const tarch::la::Vector<4, int> vector4 = {2, 2, 3, 4};

    THEN("Some entries equal, exceed and undercut the bounds two and four and every entry lies between them") {
      VALIDATE(not tarch::la::equals(vector4, 2));
      VALIDATE(tarch::la::oneEquals(vector4, 2));
      VALIDATE(tarch::la::oneGreater(vector4, 2));
      VALIDATE(tarch::la::oneGreaterEquals(vector4, 4));
      VALIDATE(not tarch::la::oneSmaller(vector4, 2));
      VALIDATE(tarch::la::oneSmallerEquals(vector4, 2));
      VALIDATE(not tarch::la::allGreater(vector4, 2));
      VALIDATE(tarch::la::allGreaterEquals(vector4, 2));
      VALIDATE(not tarch::la::allSmaller(vector4, 4));
      VALIDATE(tarch::la::allSmallerEquals(vector4, 4));
    }
  }

  GIVEN("A vector of three entries within 1e-15 of one") {
    const tarch::la::Vector<3, double> tolerant = {1.0 + 1e-15, 1.0 - 1e-15, 1.0};

    THEN("Every entry equals one within the tolerance 1e-14") {
      VALIDATE(tarch::la::equals(tolerant, 1.0, 1e-14));
      VALIDATE(tarch::la::oneEquals(tolerant, 1.0, 1e-14));
      VALIDATE(tarch::la::allGreaterEquals(tolerant, 1.0, 1e-14));
      VALIDATE(tarch::la::allSmallerEquals(tolerant, 1.0, 1e-14));
    }
  }

  GIVEN("The generic vector 2, 2, 3, 4, 5") {
    const tarch::la::Vector<5, int> generic5Compare = {2, 2, 3, 4, 5};

    THEN("Some entries equal, exceed and undercut the bounds two and five and every entry lies between them") {
      VALIDATE(not tarch::la::equals(generic5Compare, 2));
      VALIDATE(tarch::la::oneEquals(generic5Compare, 2));
      VALIDATE(tarch::la::oneGreater(generic5Compare, 4));
      VALIDATE(tarch::la::oneGreaterEquals(generic5Compare, 5));
      VALIDATE(not tarch::la::oneSmaller(generic5Compare, 2));
      VALIDATE(tarch::la::oneSmallerEquals(generic5Compare, 2));
      VALIDATE(not tarch::la::allGreater(generic5Compare, 2));
      VALIDATE(tarch::la::allGreaterEquals(generic5Compare, 2));
      VALIDATE(not tarch::la::allSmaller(generic5Compare, 5));
      VALIDATE(tarch::la::allSmallerEquals(generic5Compare, 5));
    }
  }

  GIVEN("A generic vector of seven entries within 1e-15 of one") {
    const tarch::la::Vector<7, double> generic7Compare = {1.0, 1.0 + 1e-15, 1.0 - 1e-15, 1.0, 1.0, 1.0, 1.0};

    THEN("Every entry equals one within the tolerance 1e-14") {
      VALIDATE(tarch::la::equals(generic7Compare, 1.0, 1e-14));
      VALIDATE(tarch::la::oneEquals(generic7Compare, 1.0, 1e-14));
      VALIDATE(tarch::la::allGreaterEquals(generic7Compare, 1.0, 1e-14));
      VALIDATE(tarch::la::allSmallerEquals(generic7Compare, 1.0, 1e-14));
    }
  }
}

UNIT_TEST("The sum and the difference of two vectors work entry by entry") {
  GIVEN("A vector of three ones") {
    tarch::la::Vector<3, int> vector(1);

    WHEN("The vector adds itself in place") {
      vector += vector;

      THEN("Every entry is two") {
        VALIDATE_EQUALS(vector[0], 2);
        VALIDATE_EQUALS(vector[1], 2);
        VALIDATE_EQUALS(vector[2], 2);
      }

      AND_WHEN("The vector subtracts itself in place") {
        vector -= vector;

        THEN("Every entry is zero") {
          VALIDATE_EQUALS(vector[0], 0);
          VALIDATE_EQUALS(vector[1], 0);
          VALIDATE_EQUALS(vector[2], 0);
        }
      }
    }
  }

  GIVEN("The vector 1, 2, 3") {
    tarch::la::Vector<3, int> vector = {1, 2, 3};

    WHEN("The vector takes its sum with itself") {
      vector = vector + vector;

      THEN("The vector is 2, 4, 6") {
        VALIDATE_EQUALS(vector[0], 2);
        VALIDATE_EQUALS(vector[1], 4);
        VALIDATE_EQUALS(vector[2], 6);
      }
    }

    WHEN("The vector takes its difference with a copy of itself") {
      const tarch::la::Vector<3, int> copied = vector;
      vector                                 = vector - copied;

      THEN("Every entry is zero") {
        VALIDATE_EQUALS(vector[0], 0);
        VALIDATE_EQUALS(vector[1], 0);
        VALIDATE_EQUALS(vector[2], 0);
      }
    }
  }
}

UNIT_TEST("The dot product and the cross product of two vectors follow their definitions") {
  GIVEN("The vectors 1, 2, 3 and 2, 3, 4") {
    const tarch::la::Vector<3, int> first  = {1, 2, 3};
    const tarch::la::Vector<3, int> second = {2, 3, 4};

    WHEN("The test takes the dot product of the second vector with itself by function and by operator") {
      const int dotResult         = tarch::la::dot(second, second);
      const int dotResultOperator = second * second;

      THEN("Both dot products are 29") {
        VALIDATE_EQUALS(dotResult, 29);
        VALIDATE_EQUALS(dotResultOperator, 29);
      }
    }

    WHEN("The test takes the cross product of the two vectors") {
      const tarch::la::Vector<3, int> crossResult = tarch::la::cross(first, second);

      THEN("Every entry is the difference of the products of the other two entries") {
        VALIDATE_EQUALS(crossResult[0], first[1] * second[2] - first[2] * second[1]);
        VALIDATE_EQUALS(crossResult[1], first[2] * second[0] - first[0] * second[2]);
        VALIDATE_EQUALS(crossResult[2], first[0] * second[1] - first[1] * second[0]);
      }
    }
  }
}

UNIT_TEST("The comparisons of two vectors test one entry, every entry or the first differing entry") {
  GIVEN("The vector 1, 2, 3 and a copy of it") {
    const tarch::la::Vector<3, double> first = {1.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> second(first);

    THEN("The vectors are equal and of the greater comparisons only oneGreaterEquals() is true") {
      VALIDATE(tarch::la::equals(first, second));
      VALIDATE(not tarch::la::oneGreater(first, second));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(not tarch::la::allGreater(first, second));
      VALIDATE(not tarch::la::firstGreater(first, second));
    }
  }

  GIVEN("The vectors 2, 2, 3 and 1, 2, 3") {
    const tarch::la::Vector<3, double> first  = {2.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> second = {1.0, 2.0, 3.0};

    THEN("The first vector is greater in its first entry only") {
      VALIDATE(not tarch::la::equals(first, second));
      VALIDATE(tarch::la::oneGreater(first, second));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(not tarch::la::allGreater(first, second));
      VALIDATE(tarch::la::firstGreater(first, second));
    }
  }

  GIVEN("The vectors 2, 3, 4 and 1, 2, 3") {
    const tarch::la::Vector<3, double> first  = {2.0, 3.0, 4.0};
    const tarch::la::Vector<3, double> second = {1.0, 2.0, 3.0};

    THEN("The first vector is greater in every entry") {
      VALIDATE(not tarch::la::equals(first, second));
      VALIDATE(tarch::la::oneGreater(first, second));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(tarch::la::allGreater(first, second));
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and a copy whose last entry is 0.99 tolerances larger") {
    constexpr double                   Tolerance = 1e-14;
    const tarch::la::Vector<3, double> second    = {1.0, 2.0, 3.0};
    tarch::la::Vector<3, double>       first;
    first(0) = second(0);
    first(1) = second(1);
    first(2) = second(2) + 0.99 * Tolerance;

    THEN("The vectors are equal within the tolerance and of the greater comparisons only oneGreaterEquals() is true") {
      VALIDATE(tarch::la::equals(first, second, Tolerance), first, second, first(2) - second(2));
      VALIDATE(not tarch::la::oneGreater(first, second, Tolerance));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(not tarch::la::allGreater(first, second, Tolerance));
      VALIDATE(not tarch::la::firstGreater(first, second, Tolerance), first, second);
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and a copy whose last entry is ten tolerances larger") {
    constexpr double                   Tolerance = 1e-14;
    const tarch::la::Vector<3, double> second    = {1.0, 2.0, 3.0};
    tarch::la::Vector<3, double>       first     = second;
    first(2)                                     = second(2) + 10.0 * Tolerance;

    THEN("The copy is greater in its last entry only") {
      VALIDATE(not tarch::la::equals(first, second, Tolerance));
      VALIDATE(tarch::la::oneGreater(first, second, Tolerance));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(not tarch::la::allGreater(first, second, Tolerance));
      VALIDATE(tarch::la::firstGreater(first, second, Tolerance), first, second, first(2) - second(2));
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and a copy whose entries are ten tolerances larger") {
    constexpr double                   Tolerance = 1e-14;
    const tarch::la::Vector<3, double> second    = {1.0, 2.0, 3.0};
    tarch::la::Vector<3, double>       first     = {1.0, 2.0, 3.0};
    first += 10.0 * Tolerance;

    THEN("The copy is greater in every entry") {
      VALIDATE(not tarch::la::equals(first, second, Tolerance));
      VALIDATE(tarch::la::oneGreater(first, second, Tolerance));
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
      VALIDATE(tarch::la::allGreater(first, second, Tolerance));
      VALIDATE(tarch::la::firstGreater(first, second, Tolerance));
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and a copy whose entries are 0.99 tolerances smaller") {
    constexpr double                   Tolerance = 1e-14;
    const tarch::la::Vector<3, double> second    = {1.0, 2.0, 3.0};
    tarch::la::Vector<3, double>       first     = {1.0, 2.0, 3.0};
    first -= 0.99 * Tolerance;

    THEN("One entry of the copy is greater than or equal to the entry of the vector within the default tolerance") {
      VALIDATE(tarch::la::oneGreaterEquals(first, second));
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and the vectors 1, 1, 4 and 2, 0, 0 and 1, 2 plus ten tolerances, 2") {
    constexpr double                   Tolerance              = 1e-14;
    const tarch::la::Vector<3, double> second                 = {1.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> smallerInSecondEntry   = {1.0, 1.0, 4.0};
    const tarch::la::Vector<3, double> greaterInFirstEntry    = {2.0, 0.0, 0.0};
    const tarch::la::Vector<3, double> greaterBeyondTolerance = {1.0, 2.0 + 10.0 * Tolerance, 2.0};

    THEN("The first differing entry decides which vector is greater") {
      VALIDATE(not tarch::la::firstGreater(smallerInSecondEntry, second));
      VALIDATE(tarch::la::firstGreater(greaterInFirstEntry, second));
      VALIDATE(tarch::la::firstGreater(greaterBeyondTolerance, second, Tolerance));
    }
  }

  GIVEN("The tolerance 1e-14, the vector 1, 2, 3 and a vector whose last entry is 0.99 tolerances larger") {
    constexpr double                   Tolerance = 1e-14;
    const tarch::la::Vector<3, double> first     = {1.0, 2.0, 3.0 + 0.99 * Tolerance};
    const tarch::la::Vector<3, double> second    = {1.0, 2.0, 3.0};

    WHEN("The test takes the index of the first entry in which the vectors differ") {
      const int index = tarch::la::equalsReturnIndex(first, second, Tolerance);

      THEN("equalsReturnIndex() returns the index -1") { VALIDATE_EQUALS(index, -1); }
    }
  }
}

UNIT_TEST("The vector comparison treats vectors within its tolerance as equal and orders the others") {
  GIVEN("The comparison with the tolerance 1e-12, the vector 1, 2, a vector within 1e-14 of it and the vector 1, 3") {
    const tarch::la::VectorCompare<2> compare(1.0e-12);

    const tarch::la::Vector<2, double> left  = {1.0, 2.0};
    const tarch::la::Vector<2, double> same  = {1.0 + 1.0e-14, 2.0};
    const tarch::la::Vector<2, double> right = {1.0, 3.0};

    THEN("The close vectors are incomparable and the vector 1, 2 precedes the vector 1, 3") {
      VALIDATE(not compare(left, same));
      VALIDATE(not compare(same, left));
      VALIDATE(compare(left, right));
      VALIDATE(not compare(right, left));
    }
  }
}

UNIT_TEST("The vector comparison orders the keys of a map") {
  GIVEN("An empty map ordered by the vector comparison and the vectors 0, 0 and 0, 0.3 and 1, 0") {
    std::map<tarch::la::Vector<2, double>, int, tarch::la::VectorCompare<2>> map;

    const tarch::la::Vector<2, double> smallestKey = {0.0, 0.0};
    const tarch::la::Vector<2, double> middleKey   = {0.0, 0.3};
    const tarch::la::Vector<2, double> largestKey  = {1.0, 0.0};

    THEN("The map is empty") { VALIDATE(map.empty()); }

    WHEN("The map stores the values one, two and three under the three vectors") {
      map[smallestKey] = 1;
      map[middleKey]   = 2;
      map[largestKey]  = 3;

      THEN("The map contains three keys, finds every vector and starts with the smallest vector") {
        VALIDATE_EQUALS(static_cast<int>(map.size()), 3);
        VALIDATE(map.find(smallestKey) != map.end());
        VALIDATE(map.find(middleKey) != map.end());
        VALIDATE(map.find(largestKey) != map.end());
        VALIDATE_EQUALS(map.begin()->second, 1);
      }
    }
  }
}

UNIT_TEST("The vectors of one to four entries store their entries in compact layouts") {
  GIVEN("The vectors 1.5, 2.5 and 4, 5, 6 and a vector of four sevens") {
    const tarch::la::Vector<2, double> vector2 = {1.5, 2.5};
    const tarch::la::Vector<3, int>    vector3 = {4, 5, 6};
    const tarch::la::Vector<4, int>    vector4(7);

    THEN("Every layout is trivially copyable with its size and alignment and every vector stores its entries") {
      static_assert(std::is_trivially_copyable_v<tarch::la::Vector<1, int>>);
      static_assert(std::is_trivially_copyable_v<tarch::la::Vector<2, int>>);
      static_assert(std::is_trivially_copyable_v<tarch::la::Vector<1, double>>);
      static_assert(std::is_trivially_copyable_v<tarch::la::Vector<2, double>>);
      static_assert(sizeof(tarch::la::Vector<2, int>) == 8);
      static_assert(alignof(tarch::la::Vector<2, int>) == alignof(int));
      static_assert(alignof(tarch::la::Vector<2, float>) == alignof(float));
      static_assert(std::is_standard_layout_v<tarch::la::Vector<2, int>>);
      static_assert(sizeof(tarch::la::Vector<2, double>) == 16);
      static_assert(sizeof(tarch::la::Vector<3, int>) == 12);
      static_assert(alignof(tarch::la::Vector<3, int>) == alignof(int));
      static_assert(std::is_standard_layout_v<tarch::la::Vector<3, int>>);
      static_assert(sizeof(tarch::la::Vector<3, double>) == 24);
      static_assert(sizeof(tarch::la::Vector<4, int>) == 16);
      static_assert(alignof(tarch::la::Vector<4, int>) == alignof(int));
      static_assert(alignof(tarch::la::Vector<4, double>) == alignof(double));
      static_assert(std::is_standard_layout_v<tarch::la::Vector<4, int>>);
      static_assert(sizeof(tarch::la::Vector<4, double>) == 32);
      static_assert(std::is_trivially_copyable_v<tarch::la::Vector<3, double>>);
      static_assert(sizeof(tarch::la::VectorCompare<2>) == sizeof(double));

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
      static_assert(sycl::is_device_copyable_v<tarch::la::Vector<1, int>>);
      static_assert(sycl::is_device_copyable_v<tarch::la::Vector<2, int>>);
      static_assert(sycl::is_device_copyable_v<tarch::la::Vector<1, double>>);
      static_assert(sycl::is_device_copyable_v<tarch::la::Vector<2, double>>);
#endif

      VALIDATE_EQUALS(vector2.getSize(), 2);
      VALIDATE_EQUALS(vector2.getData()[0], 1.5);
      VALIDATE_EQUALS(vector2.getData()[1], 2.5);
      VALIDATE_EQUALS(vector3.getSize(), 3);
      VALIDATE_EQUALS(vector3.getData()[0], 4);
      VALIDATE_EQUALS(vector3.getData()[1], 5);
      VALIDATE_EQUALS(vector3.getData()[2], 6);
      VALIDATE_EQUALS(vector3[0], 4);
      VALIDATE_EQUALS(vector3[1], 5);
      VALIDATE_EQUALS(vector3[2], 6);
      VALIDATE_EQUALS(vector4.getSize(), 4);
      VALIDATE_EQUALS(vector4.getData()[0], 7);
      VALIDATE_EQUALS(vector4.getData()[1], 7);
      VALIDATE_EQUALS(vector4.getData()[2], 7);
      VALIDATE_EQUALS(vector4.getData()[3], 7);
    }
  }
}

UNIT_TEST("The norms, the volume, the average and the largest magnitude of a vector reduce its entries") {
  GIVEN("The vector -1.2, 2, 3") {
    const tarch::la::Vector<3, double> vector = {-1.2, 2.0, 3.0};

    WHEN("The test takes the reductions of the vector") {
      const double squaredNorm      = tarch::la::norm2Squared(vector);
      const double norm             = tarch::la::norm2(vector);
      const double maximumNorm      = tarch::la::normMax(vector);
      const double volume           = tarch::la::volume(vector);
      const double average          = tarch::la::average(vector);
      const double largestMagnitude = tarch::la::maxAbs(vector);

      THEN("The squared 2-norm is 14.44, the 2-norm its root and the maximum norm and largest magnitude three") {
        VALIDATE_NUMERICAL_EQUALS(squaredNorm, 14.44);
        VALIDATE_NUMERICAL_EQUALS(norm, std::sqrt(14.44));
        VALIDATE_NUMERICAL_EQUALS(maximumNorm, 3.0);
        VALIDATE_NUMERICAL_EQUALS(largestMagnitude, 3.0);
      }

      THEN("The volume is -7.2 and the average 1.2666666666666666") {
        VALIDATE_NUMERICAL_EQUALS(volume, -7.2);
        VALIDATE_NUMERICAL_EQUALS(average, 1.2666666666666666);
      }
    }

    WHEN("The test takes the absolute value and the floor of the vector") {
      const auto                         absolute = tarch::la::abs(vector);
      const tarch::la::Vector<3, double> floored  = tarch::la::floor(vector);

      THEN("The absolute value is 1.2, 2, 3 and the floor -2, 2, 3") {
        VALIDATE_NUMERICAL_EQUALS(absolute[0], 1.2);
        VALIDATE_NUMERICAL_EQUALS(absolute[1], 2.0);
        VALIDATE_NUMERICAL_EQUALS(absolute[2], 3.0);
        VALIDATE_EQUALS(floored[0], -2.0);
        VALIDATE_EQUALS(floored[1], 2.0);
        VALIDATE_EQUALS(floored[2], 3.0);
      }
    }
  }

  GIVEN("The vector 2, 4, 0.5") {
    const tarch::la::Vector<3, double> vector = {2.0, 4.0, 0.5};

    WHEN("The test inverts the entries") {
      const tarch::la::Vector<3, double> inverted = tarch::la::invertEntries(vector);

      THEN("The vector stores 0.5, 0.25, 2") {
        VALIDATE_NUMERICAL_EQUALS(inverted[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(inverted[1], 0.25);
        VALIDATE_NUMERICAL_EQUALS(inverted[2], 2.0);
      }
    }
  }
}

UNIT_TEST("The predicates of a vector test its entries for a value, for equality, for finiteness and for NaN") {
  GIVEN("The vectors 1, 2, 3 and 1, 2, 2, 3 and 5, 5, 5 and 5, 6, 5, a finite vector and a vector with a NaN") {
    const tarch::la::Vector<3, int>    ascending = {1, 2, 3};
    const tarch::la::Vector<4, int>    twoTwos   = {1, 2, 2, 3};
    const tarch::la::Vector<3, int>    fives     = {5, 5, 5};
    const tarch::la::Vector<3, int>    oneSix    = {5, 6, 5};
    const tarch::la::Vector<3, double> finite    = {1.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> withNaN   = {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0};

    THEN("The predicates find the two, the equal entries, the finite entries and the NaN") {
      VALIDATE(tarch::la::contains(ascending, 2));
      VALIDATE(tarch::la::allEntriesAreTheSame(fives));
      VALIDATE(not tarch::la::allEntriesAreTheSame(oneSix));
      VALIDATE_EQUALS(tarch::la::isEntryFinite(finite), 1);
      VALIDATE_EQUALS(tarch::la::isEntryNan(withNaN), 1);
    }

    WHEN("The test counts the twos in the second vector") {
      const int numberOfTwos = tarch::la::count(twoTwos, 2);

      THEN("The vector stores two twos") { VALIDATE_EQUALS(numberOfTwos, 2); }
    }
  }
}

UNIT_TEST("The conversion of a vector into another scalar type keeps its entries") {
  GIVEN("The integer vector 1, 2, 3") {
    const tarch::la::Vector<3, int> ints = {1, 2, 3};

    WHEN("The vector converts into doubles") {
      const auto converted = tarch::la::convertScalar<double>(ints);

      THEN("The converted vector stores 1, 2, 3") {
        VALIDATE_EQUALS(converted[0], 1.0);
        VALIDATE_EQUALS(converted[1], 2.0);
        VALIDATE_EQUALS(converted[2], 3.0);
      }
    }
  }
}

UNIT_TEST("The comparisons of a vector of three entries with a scalar test one entry or every entry") {
  GIVEN("The integer vectors 3, 3, 3 and 1, 2, 3 and 1, 4, 3 and 1, 3, 2 and 4, 5, 6 and 3, 4, 5") {
    const tarch::la::Vector<3, int> threes    = {3, 3, 3};
    const tarch::la::Vector<3, int> ascending = {1, 2, 3};
    const tarch::la::Vector<3, int> withFour  = {1, 4, 3};
    const tarch::la::Vector<3, int> withThree = {1, 3, 2};
    const tarch::la::Vector<3, int> fourToSix = {4, 5, 6};
    const tarch::la::Vector<3, int> threeUp   = {3, 4, 5};

    THEN("Every comparison of one entry or of every entry with its scalar is true") {
      VALIDATE(tarch::la::equals(threes, 3));
      VALIDATE(tarch::la::oneEquals(ascending, 2));
      VALIDATE(tarch::la::oneGreater(withFour, 3));
      VALIDATE(tarch::la::oneGreaterEquals(withThree, 3));
      VALIDATE(tarch::la::oneSmaller(withFour, 2));
      VALIDATE(tarch::la::oneSmallerEquals(withFour, 1));
      VALIDATE(tarch::la::allGreater(fourToSix, 3));
      VALIDATE(tarch::la::allGreaterEquals(threeUp, 3));
      VALIDATE(tarch::la::allSmaller(ascending, 4));
      VALIDATE(tarch::la::allSmallerEquals(ascending, 3));
    }
  }

  GIVEN("The double vectors 1, 2, 3 and 2, 3, 4") {
    const tarch::la::Vector<3, double> comparedVector = {1.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> twoToFour      = {2.0, 3.0, 4.0};

    THEN("Every comparison of one entry or of every entry with its scalar is true") {
      VALIDATE(tarch::la::equals(comparedVector, 1.0, 2.1));
      VALIDATE(tarch::la::oneEquals(comparedVector, 2.0));
      VALIDATE(tarch::la::oneGreater(comparedVector, 2.5));
      VALIDATE(tarch::la::oneGreaterEquals(comparedVector, 3.0));
      VALIDATE(tarch::la::oneSmaller(comparedVector, 1.5));
      VALIDATE(tarch::la::oneSmallerEquals(comparedVector, 1.1));
      VALIDATE(tarch::la::allGreater(twoToFour, 1.5));
      VALIDATE(tarch::la::allGreaterEquals(twoToFour, 2.0));
      VALIDATE(tarch::la::allSmaller(comparedVector, 4.0));
      VALIDATE(tarch::la::allSmallerEquals(comparedVector, 3.0));
    }
  }
}

UNIT_TEST("The componentwise operations of two vectors combine the entries at equal positions") {
  GIVEN("The vectors 1, 4, 2 and 3, 1, 2 and the vectors 8, 9, 10 and 2, 3, 5") {
    const tarch::la::Vector<3, int> left     = {1, 4, 2};
    const tarch::la::Vector<3, int> right    = {3, 1, 2};
    const tarch::la::Vector<3, int> dividend = {8, 9, 10};
    const tarch::la::Vector<3, int> divisor  = {2, 3, 5};

    WHEN("The test takes the componentwise maximum, minimum, product and quotient and the first differing index") {
      const auto maxVector      = tarch::la::max(left, right);
      const auto minVector      = tarch::la::min(left, right);
      const auto productVector  = tarch::la::multiplyComponents(left, right);
      const auto quotientVector = tarch::la::divideComponents(dividend, divisor);
      const int  index          = tarch::la::equalsReturnIndex(left, right);

      THEN("Every operation combines the entries at equal positions and the vectors differ in their first entry") {
        VALIDATE_EQUALS(maxVector[0], 3);
        VALIDATE_EQUALS(maxVector[1], 4);
        VALIDATE_EQUALS(minVector[0], 1);
        VALIDATE_EQUALS(minVector[1], 1);
        VALIDATE_EQUALS(productVector[0], 3);
        VALIDATE_EQUALS(productVector[1], 4);
        VALIDATE_EQUALS(quotientVector[0], 4);
        VALIDATE_EQUALS(quotientVector[1], 3);
        VALIDATE(left != right);
        VALIDATE_EQUALS(index, 0);
      }
    }
  }

  GIVEN("The vector 5.5, -0.5, 10") {
    const tarch::la::Vector<3, double> vector = {5.5, -0.5, 10.0};

    WHEN("The test takes the remainder of every entry by two") {
      const tarch::la::Vector<3, double> remainderVector = tarch::la::remainder(vector, 2.0);

      THEN("The remainders are 1.5, 1.5 and zero") {
        VALIDATE_NUMERICAL_EQUALS(remainderVector[0], 1.5);
        VALIDATE_NUMERICAL_EQUALS(remainderVector[1], 1.5);
        VALIDATE_NUMERICAL_EQUALS(remainderVector[2], 0.0);
      }
    }
  }
}

UNIT_TEST("The vectors of two to four entries copy an array or a bit set into their entries") {
  GIVEN("Arrays of two integers, three doubles and four integers and the bit sets 10, 101 and 1101") {
    const int    raw2[] = {7, 8};
    const double raw3[] = {1.5, 2.5, 3.5};
    const int    raw4[] = {1, 2, 3, 4};

    WHEN("The test constructs vectors from the arrays and from the bit sets") {
      const tarch::la::Vector<2, int>    vector2(raw2);
      const tarch::la::Vector<2, int>    bits2(std::bitset<2>(std::string("10")));
      const tarch::la::Vector<3, double> vector3(raw3);
      const tarch::la::Vector<3, int>    bits3(std::bitset<3>(std::string("101")));
      const tarch::la::Vector<4, int>    vector4(raw4);
      const tarch::la::Vector<4, int>    bits4(std::bitset<4>(std::string("1101")));

      THEN("Every vector stores the array entries or the bits, the lowest bit first") {
        VALIDATE_EQUALS(vector2.getSize(), 2);
        VALIDATE_EQUALS(vector2.getData()[0], 7);
        VALIDATE_EQUALS(vector2(1), 8);
        VALIDATE_EQUALS(bits2[0], 0);
        VALIDATE_EQUALS(bits2[1], 1);
        VALIDATE_NUMERICAL_EQUALS(vector3.getData()[2], 3.5);
        VALIDATE_NUMERICAL_EQUALS(vector3[1], 2.5);
        VALIDATE_EQUALS(bits3[0], 1);
        VALIDATE_EQUALS(bits3[1], 0);
        VALIDATE_EQUALS(bits3[2], 1);
        VALIDATE_EQUALS(vector4.getSize(), 4);
        VALIDATE_EQUALS(vector4(3), 4);
        VALIDATE_EQUALS(bits4[0], 1);
        VALIDATE_EQUALS(bits4[1], 0);
        VALIDATE_EQUALS(bits4[2], 1);
        VALIDATE_EQUALS(bits4[3], 1);
      }
    }

    WHEN("A vector of four entries constructed from the array writes nine into its first entry") {
      tarch::la::Vector<4, int> vector4(raw4);
      vector4[0] = 9;

      THEN("The data of the vector starts with nine") { VALIDATE_EQUALS(vector4.getData()[0], 9); }
    }
  }
}

#if not defined(PEANO_WITH_FPE)
UNIT_TEST("A NaN component makes every tolerance comparison of vectors report a difference, whatever the tolerance") {
  GIVEN("The tolerance of the largest double, a finite vector, a copy with a NaN and a generic vector with a NaN") {
    constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();
    constexpr double Tolerance  = std::numeric_limits<double>::max();

    const tarch::la::Vector<3, double> finite  = {1.0, 2.0, 3.0};
    const tarch::la::Vector<3, double> withNaN = {1.0, NotANumber, 3.0};
    const tarch::la::Vector<5, double> generic = {1.0, 1.0, 1.0, NotANumber, 1.0};

    THEN("Every comparison that involves a NaN reports a difference and the finite vector equals itself") {
      VALIDATE(tarch::la::equals(finite, finite, Tolerance));
      VALIDATE(not tarch::la::equals(finite, withNaN, Tolerance));
      VALIDATE(not tarch::la::equals(withNaN, finite, Tolerance));
      VALIDATE(not tarch::la::equals(withNaN, 1.0, Tolerance));
      VALIDATE(not tarch::la::equals(generic, 1.0, Tolerance));
      VALIDATE(not tarch::la::equals(generic, generic, Tolerance));
    }

    WHEN("The test takes the indices of the first entries in which the vectors with a NaN differ") {
      const int indexOfTheCopy    = tarch::la::equalsReturnIndex(finite, withNaN, Tolerance);
      const int indexOfTheGeneric = tarch::la::equalsReturnIndex(generic, generic, Tolerance);

      THEN("Both indices are the positions of the NaN") {
        VALIDATE_EQUALS(indexOfTheCopy, 1);
        VALIDATE_EQUALS(indexOfTheGeneric, 3);
      }
    }
  }
}
#endif

UNIT_TEST("An entry below a value by less than the tolerance is smaller or equal but not smaller, for every size") {
  GIVEN("The tolerance 1e-10, values half and two tolerances around one and vectors of two to five entries") {
    constexpr double Tolerance   = 1.0e-10;
    constexpr double InsideAbove = 1.0 + 0.5 * Tolerance;
    constexpr double InsideBelow = 1.0 - 0.5 * Tolerance;
    constexpr double BeyondAbove = 1.0 + 2.0 * Tolerance;
    constexpr double BeyondBelow = 1.0 - 2.0 * Tolerance;

    const tarch::la::Vector<2, double> vector2 = {1.0, 2.0};
    const tarch::la::Vector<3, double> vector3 = {1.0, 2.0, 3.0};
    const tarch::la::Vector<4, double> vector4 = {1.0, 2.0, 3.0, 4.0};
    const tarch::la::Vector<5, double> vector5 = {1.0, 2.0, 3.0, 4.0, 5.0};

    THEN("The first entry one is smaller than the value beyond the tolerance above one only") {
      VALIDATE(not tarch::la::oneSmaller(vector2, InsideAbove, Tolerance));
      VALIDATE(not tarch::la::oneSmaller(vector3, InsideAbove, Tolerance));
      VALIDATE(not tarch::la::oneSmaller(vector4, InsideAbove, Tolerance));
      VALIDATE(not tarch::la::oneSmaller(vector5, InsideAbove, Tolerance));
      VALIDATE(tarch::la::oneSmaller(vector2, BeyondAbove, Tolerance));
      VALIDATE(tarch::la::oneSmaller(vector3, BeyondAbove, Tolerance));
      VALIDATE(tarch::la::oneSmaller(vector4, BeyondAbove, Tolerance));
      VALIDATE(tarch::la::oneSmaller(vector5, BeyondAbove, Tolerance));
    }

    THEN("The first entry one is smaller than or equal to the value within the tolerance below one only") {
      VALIDATE(tarch::la::oneSmallerEquals(vector2, InsideBelow, Tolerance));
      VALIDATE(tarch::la::oneSmallerEquals(vector3, InsideBelow, Tolerance));
      VALIDATE(tarch::la::oneSmallerEquals(vector4, InsideBelow, Tolerance));
      VALIDATE(tarch::la::oneSmallerEquals(vector5, InsideBelow, Tolerance));
      VALIDATE(not tarch::la::oneSmallerEquals(vector2, BeyondBelow, Tolerance));
      VALIDATE(not tarch::la::oneSmallerEquals(vector3, BeyondBelow, Tolerance));
      VALIDATE(not tarch::la::oneSmallerEquals(vector4, BeyondBelow, Tolerance));
      VALIDATE(not tarch::la::oneSmallerEquals(vector5, BeyondBelow, Tolerance));
    }
  }
}

UNIT_TEST("Every order of the components of a 2D or a 3D vector keeps its squared norm to the last bit") {
  GIVEN("Two thousand steps along a line of planar vectors and along a line of spatial vectors") {
    constexpr int NumberOfSteps      = 2000;
    constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

    WHEN("The test takes the squared norm of every vector and of every order of its components") {
      std::vector<double> planeNorms;
      std::vector<double> swappedPlaneNorms;
      std::vector<double> spaceNorms;
      std::vector<double> permutedSpaceNorms;
      for (int step = 0; step < NumberOfSteps; step++) {
        tarch::la::Vector<2, double> plane;
        plane(0) = -0.298 + 0.001 * step;
        plane(1) = -0.29676 + 0.000617 * step;
        tarch::la::Vector<2, double> swappedPlane;
        swappedPlane(0) = plane(1);
        swappedPlane(1) = plane(0);
        planeNorms.push_back(tarch::la::norm2Squared(plane));
        swappedPlaneNorms.push_back(tarch::la::norm2Squared(swappedPlane));

        tarch::la::Vector<3, double> space;
        space(0) = plane(0);
        space(1) = plane(1);
        space(2) = 0.173 + 0.000431 * step;
        for (const auto& order : Permutations) {
          tarch::la::Vector<3, double> permutedSpace;
          for (int component = 0; component < 3; component++) {
            permutedSpace(component) = space(order[component]);
          }
          spaceNorms.push_back(tarch::la::norm2Squared(space));
          permutedSpaceNorms.push_back(tarch::la::norm2Squared(permutedSpace));
        }
      }

      THEN("Every vector has the squared norm of every order of its components bit for bit") {
        for (std::size_t step = 0; step < planeNorms.size(); step++) {
          VALIDATE_EQUALS(planeNorms[step], swappedPlaneNorms[step], step);
        }
        for (std::size_t entry = 0; entry < spaceNorms.size(); entry++) {
          VALIDATE_EQUALS(spaceNorms[entry], permutedSpaceNorms[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("The squared norm adds the squares in the order of ascending magnitude") {
  GIVEN("A spatial vector with the components 1e8, 1 and 1") {
    // The squares 1, 1 and 1e16 sum to 1e16 + 2 only in the order of ascending magnitude.
    tarch::la::Vector<3, double> unbalanced;
    unbalanced(0) = 1.0e8;
    unbalanced(1) = 1.0;
    unbalanced(2) = 1.0;

    WHEN("The test takes its squared norm") {
      const double squaredNorm = tarch::la::norm2Squared(unbalanced);

      THEN("The squared norm is 1e16 + 2") { VALIDATE_EQUALS(squaredNorm, 1.0e16 + 2.0); }
    }
  }
}
