// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Scalar.h"

#include "tarch/la/ScalarOperations.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("Scalar comparisons treat two values within the tolerance as equal") {
  GIVEN("The value one, the tolerance 1e-14 and values one tenth, one and ten tolerances away from one") {
    constexpr double Value                  = 1.0;
    constexpr double Tolerance              = 1.0e-14;
    constexpr double OneTenthToleranceAbove = Value + 0.1 * Tolerance;
    constexpr double OneToleranceAbove      = Value + Tolerance;
    constexpr double OneToleranceBelow      = Value - Tolerance;
    constexpr double TenTolerancesAbove     = Value + 10.0 * Tolerance;
    constexpr double TenTolerancesBelow     = Value - 10.0 * Tolerance;

    THEN("One equals itself and every value at most one tolerance away") {
      VALIDATE(tarch::la::equals(Value, Value, Tolerance));
      VALIDATE(tarch::la::equals(Value, OneToleranceAbove, Tolerance));
      VALIDATE(tarch::la::greaterEquals(Value, Value, Tolerance));
      VALIDATE(tarch::la::greaterEquals(Value, OneTenthToleranceAbove, Tolerance));
      VALIDATE(tarch::la::smallerEquals(Value, OneToleranceAbove, Tolerance));
      VALIDATE(tarch::la::smallerEquals(OneToleranceAbove, Value, Tolerance));
      VALIDATE(not tarch::la::greater(Value, OneToleranceBelow, Tolerance));
      VALIDATE(not tarch::la::smaller(Value, OneToleranceAbove, Tolerance));
    }

    THEN("One differs from the values ten tolerances away and lies between them") {
      VALIDATE(not tarch::la::equals(Value, TenTolerancesAbove, Tolerance));
      VALIDATE(tarch::la::greater(Value, TenTolerancesBelow, Tolerance));
      VALIDATE(tarch::la::smaller(Value, TenTolerancesAbove, Tolerance));
      VALIDATE(tarch::la::smallerEquals(Value, TenTolerancesAbove, Tolerance));
      VALIDATE(not tarch::la::greaterEquals(Value, TenTolerancesAbove, Tolerance));
    }

    THEN("One equals the value ten tolerances above it under a tolerance ten times larger") {
      VALIDATE(tarch::la::equals(Value, TenTolerancesAbove, 10.0 * Tolerance));
    }

    THEN("One is smaller than two") {
      VALIDATE(tarch::la::greater(2.0, Value, Tolerance));
      VALIDATE(tarch::la::greaterEquals(2.0, Value, Tolerance));
      VALIDATE(tarch::la::smaller(Value, 2.0, Tolerance));
      VALIDATE(tarch::la::smallerEquals(Value, 2.0, Tolerance));
      VALIDATE(not tarch::la::greaterEquals(Value, 2.0, Tolerance));
      VALIDATE(not tarch::la::equals(Value, 2.0, Tolerance));
    }
  }
}

UNIT_TEST("The absolute value of a negative number is its negation") {
  GIVEN("Minus one as a double and as an integer") {
    constexpr double DoubleValue  = -1.0;
    constexpr int    IntegerValue = -1;

    WHEN("The test takes the absolute values") {
      const double absoluteDoubleValue  = tarch::la::abs(DoubleValue);
      const int    absoluteIntegerValue = tarch::la::abs(IntegerValue);

      THEN("Both absolute values are one") {
        VALIDATE_EQUALS(absoluteDoubleValue, 1.0);
        VALIDATE_EQUALS(absoluteIntegerValue, 1);
      }
    }
  }
}

UNIT_TEST("The sign of a value is zero within the tolerance and plus or minus one beyond it") {
  GIVEN("The values two and minus two beyond the default tolerance 1e-8") {
    constexpr double Positive = 2.0;
    constexpr double Negative = -2.0;

    WHEN("The test takes their signs with the default tolerance") {
      const int signOfPositive = tarch::la::sign(Positive);
      const int signOfNegative = tarch::la::sign(Negative);

      THEN("The signs are one and minus one") {
        VALIDATE_EQUALS(signOfPositive, 1);
        VALIDATE_EQUALS(signOfNegative, -1);
      }
    }
  }

  GIVEN("Zero and the values 1e-10 and -1e-10 within the default tolerance 1e-8") {
    constexpr double Zero          = 0.0;
    constexpr double SmallPositive = 1.0e-10;
    constexpr double SmallNegative = -1.0e-10;

    WHEN("The test takes their signs with the default tolerance") {
      const int signOfZero          = tarch::la::sign(Zero);
      const int signOfSmallPositive = tarch::la::sign(SmallPositive);
      const int signOfSmallNegative = tarch::la::sign(SmallNegative);

      THEN("Every sign is zero") {
        VALIDATE_EQUALS(signOfZero, 0);
        VALIDATE_EQUALS(signOfSmallPositive, 0);
        VALIDATE_EQUALS(signOfSmallNegative, 0);
      }
    }
  }

  GIVEN("The tolerance one and the values 0.5, -0.5, 1.5 and -1.5") {
    constexpr double Tolerance      = 1.0;
    constexpr double PositiveWithin = 0.5;
    constexpr double NegativeWithin = -0.5;
    constexpr double PositiveBeyond = 1.5;
    constexpr double NegativeBeyond = -1.5;

    WHEN("The test takes their signs with this tolerance") {
      const int signOfPositiveWithin = tarch::la::sign(PositiveWithin, Tolerance);
      const int signOfNegativeWithin = tarch::la::sign(NegativeWithin, Tolerance);
      const int signOfPositiveBeyond = tarch::la::sign(PositiveBeyond, Tolerance);
      const int signOfNegativeBeyond = tarch::la::sign(NegativeBeyond, Tolerance);

      THEN("The values within the tolerance give zero and the values beyond it keep their sign") {
        VALIDATE_EQUALS(signOfPositiveWithin, 0);
        VALIDATE_EQUALS(signOfNegativeWithin, 0);
        VALIDATE_EQUALS(signOfPositiveBeyond, 1);
        VALIDATE_EQUALS(signOfNegativeBeyond, -1);
      }
    }
  }
}

UNIT_TEST("Rounding picks the nearest integer") {
  GIVEN("The values 2.4 and 2.6 in double precision and 2.6 in single precision") {
    constexpr double RoundsDown                = 2.4;
    constexpr double RoundsUp                  = 2.6;
    constexpr float  RoundsUpInSinglePrecision = 2.6F;

    WHEN("The test rounds the values") {
      const int roundedDown                = tarch::la::round(RoundsDown);
      const int roundedUp                  = tarch::la::round(RoundsUp);
      const int roundedUpInSinglePrecision = tarch::la::round(RoundsUpInSinglePrecision);

      THEN("The value 2.4 rounds to two and the value 2.6 rounds to three in both precisions") {
        VALIDATE_EQUALS(roundedDown, 2);
        VALIDATE_EQUALS(roundedUp, 3);
        VALIDATE_EQUALS(roundedUpInSinglePrecision, 3);
      }
    }
  }
}

UNIT_TEST("The maximum and the minimum of three values are the largest and the smallest value") {
  GIVEN("The values one, three and two") {
    constexpr double First  = 1.0;
    constexpr double Second = 3.0;
    constexpr double Third  = 2.0;

    WHEN("The test takes their maximum and minimum") {
      const double maximum = tarch::la::max(First, Second, Third);
      const double minimum = tarch::la::min(First, Second, Third);

      THEN("The maximum is three and the minimum is one") {
        VALIDATE_EQUALS(maximum, 3.0);
        VALIDATE_EQUALS(minimum, 1.0);
      }
    }
  }
}

UNIT_TEST("The power of a scalar raises its base to the exponent") {
  GIVEN("The base four and the exponent one half") {
    constexpr double Base     = 4.0;
    constexpr double Exponent = 0.5;

    WHEN("The test takes the power") {
      const double power = tarch::la::pow(Base, Exponent);

      THEN("The power is two") { VALIDATE_NUMERICAL_EQUALS(power, 2.0); }
    }
  }
}

UNIT_TEST("The order and magnitude keys and the comparisons follow the order of the numbers with both zeros equal") {
  GIVEN("Twelve doubles and nine floats from minus infinity over both zeros and the subnormal extremes to infinity") {
    constexpr double DoubleInfinity = std::numeric_limits<double>::infinity();
    constexpr float  FloatInfinity  = std::numeric_limits<float>::infinity();
    const double     doubles[] =
      {-DoubleInfinity, -1.0e300, -2.0, -1.0, -0x1p-1074, -0.0, 0.0, 0x1p-1074, 1.0, 2.0, 1.0e300, DoubleInfinity};
    const float floats[] = {-FloatInfinity, -2.0F, -0x1p-149F, -0.0F, 0.0F, 0x1p-149F, 1.0F, 3.0e38F, FloatInfinity};
    const auto  validateEveryPair = [](const auto& values) {
      for (const auto lhs : values) {
        for (const auto rhs : values) {
          const bool lhsIsBelow            = lhs < rhs;
          const bool lhsIsBelowInMagnitude = std::abs(lhs) < std::abs(rhs);
          const bool magnitudesAreEqual    = std::abs(lhs) == std::abs(rhs);
          VALIDATE((tarch::la::getOrderKeyOf(lhs) < tarch::la::getOrderKeyOf(rhs)) == lhsIsBelow, lhs, rhs);
          VALIDATE(
            (tarch::la::getMagnitudeKeyOf(lhs) < tarch::la::getMagnitudeKeyOf(rhs)) == lhsIsBelowInMagnitude,
            lhs,
            rhs
          );
          VALIDATE(
            (tarch::la::getMagnitudeKeyOf(lhs) == tarch::la::getMagnitudeKeyOf(rhs)) == magnitudesAreEqual,
            lhs,
            rhs
          );
          VALIDATE(tarch::la::isBelow(lhs, rhs) == lhsIsBelow, lhs, rhs);
          VALIDATE(tarch::la::isBelowInMagnitude(lhs, rhs) == lhsIsBelowInMagnitude, lhs, rhs);
          VALIDATE(tarch::la::isEqualInMagnitude(lhs, rhs) == magnitudesAreEqual, lhs, rhs);
        }
      }
    };

    THEN("The keys and the comparisons of every pair agree with the comparison of the two numbers") {
      validateEveryPair(doubles);
      validateEveryPair(floats);
    }

    THEN("The comparisons order integers as well") {
      VALIDATE(tarch::la::isBelow(-3, 2));
      VALIDATE(tarch::la::isEqualInMagnitude(-4, 4));
    }
  }
}

UNIT_TEST(
  "productRoundedOnce() returns the rounded product of doubles, floats and integers and positive zero for a zero factor"
) {
  GIVEN("The products of 0.1 and 0.7 in double and in single precision") {
    const double doubleProduct = 0.1 * 0.7;
    const float  floatProduct  = 0.1F * 0.7F;

    THEN("The product rounded once returns these products, -21 for -3 times 7 and positive zero for a zero factor") {
      VALIDATE_EQUALS(tarch::la::productRoundedOnce(0.1, 0.7), doubleProduct);
      VALIDATE_EQUALS(tarch::la::productRoundedOnce(0.1F, 0.7F), floatProduct);
      VALIDATE_EQUALS(tarch::la::productRoundedOnce(-3, 7), -21);
      VALIDATE(not std::signbit(tarch::la::productRoundedOnce(-0.0, 2.0)));
      VALIDATE(not std::signbit(tarch::la::productRoundedOnce(0.0F, -2.0F)));
    }
  }
}

UNIT_TEST(
  "Every order of three arguments returns the bits of the sum and the product in the order of ascending magnitude"
) {
  GIVEN("Summands whose sum and factors whose product depend on the order and three values of distinct magnitude") {
    constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    // The two ones add up before the large summand. The opposite summands of equal magnitude add
    // first and cancel exactly.
    const double summands[3]           = {1.0e16, 1.0, 1.0};
    const double expectedSum           = 1.0e16 + 2.0;
    const double cancellingSummands[3] = {1.0e16, -1.0e16, 1.0 / 3.0};
    const double factors[3]            = {10.0, 3.0, 0.1};
    const double expectedProduct       = (factors[2] * factors[1]) * factors[0];
    const double orderedValues[3]      = {1.0, 2.0, -3.0};

    THEN("The sums and the product take the order of ascending magnitude and negated summands give the negated sum") {
      VALIDATE(expectedSum != (summands[0] + summands[1]) + summands[2]);
      VALIDATE(expectedProduct != (factors[1] * factors[0]) * factors[2]);
      for (const auto& order : Permutations) {
        VALIDATE_EQUALS(
          tarch::la::sumInOrderOfMagnitude(summands[order[0]], summands[order[1]], summands[order[2]]),
          expectedSum,
          order[0],
          order[1],
          order[2]
        );
        VALIDATE_EQUALS(
          tarch::la::sumInOrderOfMagnitude(-summands[order[0]], -summands[order[1]], -summands[order[2]]),
          -expectedSum,
          order[0],
          order[1],
          order[2]
        );
        VALIDATE_EQUALS(
          tarch::la::sumInOrderOfMagnitude(
            cancellingSummands[order[0]],
            cancellingSummands[order[1]],
            cancellingSummands[order[2]]
          ),
          cancellingSummands[2],
          order[0],
          order[1],
          order[2]
        );
        VALIDATE_EQUALS(
          tarch::la::productInOrderOfMagnitude(factors[order[0]], factors[order[1]], factors[order[2]]),
          expectedProduct,
          order[0],
          order[1],
          order[2]
        );
      }
    }

    THEN("orderByMagnitude() sorts the three values in every order by their magnitude") {
      for (const auto& order : Permutations) {
        double smallest = orderedValues[order[0]];
        double middle   = orderedValues[order[1]];
        double largest  = orderedValues[order[2]];
        tarch::la::orderByMagnitude(smallest, middle, largest);
        VALIDATE_EQUALS(smallest, orderedValues[0], order[0], order[1], order[2]);
        VALIDATE_EQUALS(middle, orderedValues[1], order[0], order[1], order[2]);
        VALIDATE_EQUALS(largest, orderedValues[2], order[0], order[1], order[2]);
      }
    }
  }
}

UNIT_TEST("The sum of three squares adds the two small squares first and keeps its bits for every order and sign") {
  GIVEN("The arguments 1e8, 1 and -1 and the arguments 0.3, -0.7 and 0.7 with two largest of equal magnitude") {
    constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    constexpr int SignPatterns       = 8;
    const double  argumentSets[2][3] = {{1.0e8, 1.0, -1.0}, {0.3, -0.7, 0.7}};

    THEN("Every order and every sign pattern returns the sum of the squares in the given order bitwise") {
      for (const auto& arguments : argumentSets) {
        const double sumInArgumentOrder =
          tarch::la::sumOfSquaresInOrderOfMagnitude(arguments[0], arguments[1], arguments[2]);
        for (const auto& order : Permutations) {
          for (int signs = 0; signs < SignPatterns; signs++) {
            const double first  = (signs & 1) != 0 ? -arguments[order[0]] : arguments[order[0]];
            const double second = (signs & 2) != 0 ? -arguments[order[1]] : arguments[order[1]];
            const double third  = (signs & 4) != 0 ? -arguments[order[2]] : arguments[order[2]];
            VALIDATE_EQUALS(
              tarch::la::sumOfSquaresInOrderOfMagnitude(first, second, third),
              sumInArgumentOrder,
              order[0],
              order[1],
              order[2],
              signs
            );
          }
        }
      }
    }

    THEN("The squares of 1e8, 1 and -1 sum exactly to 1e16 + 2") {
      VALIDATE(1.0e16 + 2.0 != (1.0e16 + 1.0) + 1.0);
      VALIDATE_EQUALS(
        tarch::la::sumOfSquaresInOrderOfMagnitude(argumentSets[0][0], argumentSets[0][1], argumentSets[0][2]),
        1.0e16 + 2.0
      );
    }
  }
}

UNIT_TEST("The sum of squares in order of magnitude is more exact than the sum of the squares rounded once each") {
  GIVEN("4096 triples of a sine, a cosine and a sine scaled by a power of ten from 1e-3 to 1e3") {
    constexpr int Samples = 4096;

    WHEN("Both sums run for every triple and their largest relative errors against a long double sum are taken") {
      long double largestFusedError   = 0.0L;
      long double largestRoundedError = 0.0L;
      for (int sample = 0; sample < Samples; sample++) {
        const double      first  = std::sin(1.0 + sample);
        const double      second = std::cos(2.0 + 3.0 * sample);
        const double      third  = std::sin(0.5 * sample) * std::pow(10.0, sample % 7 - 3);
        const long double exact  = static_cast<long double>(first) * first + static_cast<long double>(second) * second
          + static_cast<long double>(third) * third;
        const double fused   = tarch::la::sumOfSquaresInOrderOfMagnitude(first, second, third);
        const double rounded = tarch::la::sumInOrderOfMagnitude(
          tarch::la::productRoundedOnce(first, first),
          tarch::la::productRoundedOnce(second, second),
          tarch::la::productRoundedOnce(third, third)
        );
        largestFusedError   = std::max(largestFusedError, std::abs(fused - exact) / exact);
        largestRoundedError = std::max(largestRoundedError, std::abs(rounded - exact) / exact);
      }

      THEN("The largest error of the sum of squares in order of magnitude lies below the one of the rounded squares") {
        VALIDATE(
          largestFusedError < largestRoundedError,
          static_cast<double>(largestFusedError),
          static_cast<double>(largestRoundedError)
        );
      }
    }
  }
}

UNIT_TEST("Every order of three arguments returns the bits of the sum and of the mean in the order of value") {
  GIVEN("The values 0.1, 0.7 and 0.3, whose sum depends on the order, and a value just above 1.5") {
    constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    const double  values[3]          = {0.1, 0.7, 0.3};
    const double  equalValue         = 1.5 + 2.0 * std::numeric_limits<double>::epsilon();

    THEN("Every order returns the sum in the order of value and negated values return the negated sum") {
      const double expectedSum = (values[0] + values[1]) + values[2];
      for (const auto& order : Permutations) {
        VALIDATE_EQUALS(
          tarch::la::sumInOrderOfValue(values[order[0]], values[order[1]], values[order[2]]),
          expectedSum,
          order[0],
          order[1],
          order[2]
        );
        VALIDATE_EQUALS(
          tarch::la::sumInOrderOfValue(-values[order[0]], -values[order[1]], -values[order[2]]),
          -expectedSum,
          order[0],
          order[1],
          order[2]
        );
      }
    }

    THEN("Every order returns the same mean, negated values the negated mean and three equal values their value") {
      const double expected = tarch::la::meanInOrderOfValue(values[0], values[1], values[2]);
      VALIDATE_NUMERICAL_EQUALS(expected, (0.1 + 0.7 + 0.3) / 3.0);
      VALIDATE(((values[0] + values[1]) + values[2]) / 3.0 != ((values[0] + values[2]) + values[1]) / 3.0);
      VALIDATE(((equalValue + equalValue) + equalValue) / 3.0 != equalValue);
      VALIDATE_EQUALS(tarch::la::meanInOrderOfValue(equalValue, equalValue, equalValue), equalValue);
      for (const auto& order : Permutations) {
        VALIDATE_EQUALS(
          tarch::la::meanInOrderOfValue(values[order[0]], values[order[1]], values[order[2]]),
          expected,
          order[0],
          order[1],
          order[2]
        );
        VALIDATE_EQUALS(
          tarch::la::meanInOrderOfValue(-values[order[0]], -values[order[1]], -values[order[2]]),
          -expected,
          order[0],
          order[1],
          order[2]
        );
      }
    }
  }
}

UNIT_TEST("Every order of the entries returns the same fixed-point sum and negated entries return the negated sum") {
  GIVEN("1000 entries over seven orders of magnitude and their reversed, interleaved and negated copies") {
    constexpr int Size = 1000;
    double        values[Size];
    double        reversed[Size];
    double        interleaved[Size];
    double        negated[Size];
    double        sumInIndexOrder = 0.0;
    for (int entry = 0; entry < Size; entry++) {
      values[entry] = std::sin(1.0 + entry) * std::pow(10.0, entry % 7 - 3);
      sumInIndexOrder += values[entry];
    }
    // Seven and the size are coprime. The stride seven then visits every entry once.
    for (int entry = 0; entry < Size; entry++) {
      reversed[entry]    = values[Size - 1 - entry];
      interleaved[entry] = values[(entry * 7) % Size];
      negated[entry]     = -values[entry];
    }

    WHEN("The fixed-point sum adds the entries and every copy") {
      const double sum            = tarch::la::sumInFixedPoint(values, Size);
      const double reversedSum    = tarch::la::sumInFixedPoint(reversed, Size);
      const double interleavedSum = tarch::la::sumInFixedPoint(interleaved, Size);
      const double negatedSum     = tarch::la::sumInFixedPoint(negated, Size);

      THEN("Both orders return the sum bitwise, the negated copy its negation and the plain sum lies within 1e-10") {
        VALIDATE_EQUALS(reversedSum, sum);
        VALIDATE_EQUALS(interleavedSum, sum);
        VALIDATE_EQUALS(negatedSum, -sum);
        VALIDATE_NUMERICAL_EQUALS(sum, sumInIndexOrder);
      }
    }

    THEN("A sum of zero entries and a sum of signed zeros are zero and a sum with an infinite entry is infinite") {
      const double zeros[3]    = {0.0, -0.0, 0.0};
      const double infinite[2] = {1.0, std::numeric_limits<double>::infinity()};
      VALIDATE_EQUALS(tarch::la::sumInFixedPoint(values, 0), 0.0);
      VALIDATE_EQUALS(tarch::la::sumInFixedPoint(zeros, 3), 0.0);
      VALIDATE(std::isinf(tarch::la::sumInFixedPoint(infinite, 2)));
    }
  }
}

UNIT_TEST("Every order of the values returns the same fixed-point mean and negated values return the negated mean") {
  GIVEN("27 values over five orders of magnitude, their reversed, interleaved and negated copies and 27 times 0.1") {
    constexpr int Count = 27;
    double        values[Count];
    double        reversed[Count];
    double        interleaved[Count];
    double        negated[Count];
    double        equal[Count];
    double        meanInIndexOrder = 0.0;
    for (int entry = 0; entry < Count; entry++) {
      values[entry] = std::sin(1.0 + entry) * std::pow(10.0, entry % 5 - 2);
      meanInIndexOrder += values[entry] / Count;
    }
    // Two and the count are coprime. The stride two then visits every entry once.
    for (int entry = 0; entry < Count; entry++) {
      reversed[entry]    = values[Count - 1 - entry];
      interleaved[entry] = values[(entry * 2) % Count];
      negated[entry]     = -values[entry];
      equal[entry]       = 0.1;
    }

    WHEN("The fixed-point mean takes the values and every copy") {
      const double mean            = tarch::la::meanInFixedPoint(values);
      const double reversedMean    = tarch::la::meanInFixedPoint(reversed);
      const double interleavedMean = tarch::la::meanInFixedPoint(interleaved);
      const double negatedMean     = tarch::la::meanInFixedPoint(negated);
      const double equalMean       = tarch::la::meanInFixedPoint(equal);

      THEN("Both orders return the bits of the mean, the negated copy the negated mean and the equal values 0.1") {
        VALIDATE_EQUALS(reversedMean, mean);
        VALIDATE_EQUALS(interleavedMean, mean);
        VALIDATE_EQUALS(negatedMean, -mean);
        VALIDATE_EQUALS(equalMean, 0.1);
        VALIDATE_NUMERICAL_EQUALS(mean, meanInIndexOrder);
      }
    }
  }
}

UNIT_TEST("The fixed-point mean of values on its grid returns their exact mean rounded once") {
  GIVEN("Sixteen sets of 27 integer multiples of 2^-20 below 2^-3") {
    constexpr int Count        = 27;
    constexpr int NumberOfSets = 16;

    WHEN("The fixed-point mean and the double division of the exact sum take every set") {
      // Integer multiples of 2^-20 below 2^-3 sum exactly in a double. A double division rounds once.
      std::vector<std::uint64_t> means;
      std::vector<std::uint64_t> expectedMeans;
      for (int entry = 0; entry < NumberOfSets; entry++) {
        double values[Count];
        double exactSum = 0.0;
        for (int index = 0; index < Count; index++) {
          values[index] =
            std::ldexp(static_cast<double>((entry * 7919 + index * 104729) % 4096 - 2048), index % 7 - 20);
          exactSum += values[index];
        }
        means.push_back(std::bit_cast<std::uint64_t>(tarch::la::meanInFixedPoint(values)));
        expectedMeans.push_back(std::bit_cast<std::uint64_t>(exactSum / Count));
      }

      THEN("Every fixed-point mean has the bits of the exact sum divided by 27") {
        for (std::size_t entry = 0; entry < means.size(); entry++) {
          VALIDATE_EQUALS(means[entry], expectedMeans[entry], entry);
        }
      }
    }
  }
}
