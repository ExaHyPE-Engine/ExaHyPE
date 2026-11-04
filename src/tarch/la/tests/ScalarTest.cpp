// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ScalarTest.h"

#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"

tarch::la::tests::ScalarTest::ScalarTest() :
  TestCase("tarch::la::tests::ScalarTest") {}

void
tarch::la::tests::ScalarTest::run() {
  testMethod(testComparison);
  testMethod(testAbs);
  testMethod(testAdditionalScalarFunctions);
  testMethod(testTheKeysAndComparisonsFollowTheOrderOfTheNumbers);
  testMethod(testEveryOrderOfTheArgumentsReturnsTheSameSumAndProduct);
  testMethod(testEveryOrderAndSignOfTheArgumentsReturnsTheSameSumOfSquares);
  testMethod(testTheSumOfSquaresInOrderOfMagnitudeIsMoreExactThanTheSumOfRoundedSquares);
  testMethod(testEveryOrderOfTheArgumentsReturnsTheSameSumAndMeanInOrderOfValue);
  testMethod(testEveryOrderOfTheEntriesReturnsTheSameFixedPointSum);
  testMethod(testTheProductRoundedOnceReturnsTheRoundedProduct);
  testMethod(testEveryOrderOfTheValuesReturnsTheSameFixedPointMean);
  testMethod(testTheFixedPointMeanRoundsTheExactMeanOnce);
}

/**
 * The fixed-point mean returns the same bits for every order of the values.
 */
void
tarch::la::tests::ScalarTest::testEveryOrderOfTheValuesReturnsTheSameFixedPointMean() {
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

  const double mean = tarch::la::meanInFixedPoint(values);
  validateEquals(tarch::la::meanInFixedPoint(reversed), mean);
  validateEquals(tarch::la::meanInFixedPoint(interleaved), mean);
  validateEquals(tarch::la::meanInFixedPoint(negated), -mean);
  validateEquals(tarch::la::meanInFixedPoint(equal), 0.1);
  validateNumericalEqualsWithEps(mean, meanInIndexOrder, 1.0e-13);
}

/**
 * The fixed-point mean of values on the grid returns their exact mean rounded once.
 */
void
tarch::la::tests::ScalarTest::testTheFixedPointMeanRoundsTheExactMeanOnce() {
  constexpr int Count = 27;
  for (int entry = 0; entry < 16; entry++) {
    // Integer multiples of 2^-20 below 2^-3 sum exactly in a double. A double division rounds once.
    double values[Count];
    double exactSum = 0.0;
    for (int index = 0; index < Count; index++) {
      values[index] = std::ldexp(static_cast<double>((entry * 7919 + index * 104729) % 4096 - 2048), index % 7 - 20);
      exactSum += values[index];
    }
    validateEqualsWithParams1(
      std::bit_cast<std::uint64_t>(tarch::la::meanInFixedPoint(values)),
      std::bit_cast<std::uint64_t>(exactSum / Count),
      entry
    );
  }
}

/**
 * The order and magnitude keys and the comparisons follow the order of the numbers, with both zeros
 * equal, for every pair of doubles and of floats.
 */
void
tarch::la::tests::ScalarTest::testTheKeysAndComparisonsFollowTheOrderOfTheNumbers() {
  const auto validateEveryPair = [this](const auto& values) {
    for (const auto lhs : values) {
      for (const auto rhs : values) {
        const bool lhsIsBelow            = lhs < rhs;
        const bool lhsIsBelowInMagnitude = std::abs(lhs) < std::abs(rhs);
        const bool magnitudesAreEqual    = std::abs(lhs) == std::abs(rhs);
        validateWithParams2((tarch::la::getOrderKeyOf(lhs) < tarch::la::getOrderKeyOf(rhs)) == lhsIsBelow, lhs, rhs);
        validateWithParams2(
          (tarch::la::getMagnitudeKeyOf(lhs) < tarch::la::getMagnitudeKeyOf(rhs)) == lhsIsBelowInMagnitude,
          lhs,
          rhs
        );
        validateWithParams2(
          (tarch::la::getMagnitudeKeyOf(lhs) == tarch::la::getMagnitudeKeyOf(rhs)) == magnitudesAreEqual,
          lhs,
          rhs
        );
        validateWithParams2(tarch::la::isBelow(lhs, rhs) == lhsIsBelow, lhs, rhs);
        validateWithParams2(tarch::la::isBelowInMagnitude(lhs, rhs) == lhsIsBelowInMagnitude, lhs, rhs);
        validateWithParams2(tarch::la::isEqualInMagnitude(lhs, rhs) == magnitudesAreEqual, lhs, rhs);
      }
    }
  };
  constexpr double DoubleInfinity = std::numeric_limits<double>::infinity();
  constexpr float  FloatInfinity  = std::numeric_limits<float>::infinity();
  const double     doubles[] =
    {-DoubleInfinity, -1.0e300, -2.0, -1.0, -0x1p-1074, -0.0, 0.0, 0x1p-1074, 1.0, 2.0, 1.0e300, DoubleInfinity};
  const float floats[] = {-FloatInfinity, -2.0F, -0x1p-149F, -0.0F, 0.0F, 0x1p-149F, 1.0F, 3.0e38F, FloatInfinity};
  validateEveryPair(doubles);
  validateEveryPair(floats);
  validate(tarch::la::isBelow(-3, 2));
  validate(tarch::la::isEqualInMagnitude(-4, 4));
}

/**
 * productRoundedOnce() returns the bits of the rounded product of doubles, floats and integers and
 * positive zero for a zero factor.
 */
void
tarch::la::tests::ScalarTest::testTheProductRoundedOnceReturnsTheRoundedProduct() {
  const double doubleProduct = 0.1 * 0.7;
  const float  floatProduct  = 0.1F * 0.7F;
  validateEquals(tarch::la::productRoundedOnce(0.1, 0.7), doubleProduct);
  validateEquals(tarch::la::productRoundedOnce(0.1F, 0.7F), floatProduct);
  validateEquals(tarch::la::productRoundedOnce(-3, 7), -21);
  validateTrue(not std::signbit(tarch::la::productRoundedOnce(-0.0, 2.0)));
  validateTrue(not std::signbit(tarch::la::productRoundedOnce(0.0F, -2.0F)));
}

/**
 * Every order of three arguments returns the bits of the sum and the product taken in the order of
 * ascending magnitude, and three negated summands return the negated sum, for values whose sum and
 * product depend on the order.
 */
void
tarch::la::tests::ScalarTest::testEveryOrderOfTheArgumentsReturnsTheSameSumAndProduct() {
  constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

  // The two ones add up before the large summand. The opposite summands of equal magnitude add
  // first and cancel exactly.
  const double summands[3]           = {1.0e16, 1.0, 1.0};
  const double expectedSum           = 1.0e16 + 2.0;
  const double cancellingSummands[3] = {1.0e16, -1.0e16, 1.0 / 3.0};
  const double factors[3]            = {10.0, 3.0, 0.1};
  const double expectedProduct       = (factors[2] * factors[1]) * factors[0];
  const double orderedValues[3]      = {1.0, 2.0, -3.0};
  validate(expectedSum != (summands[0] + summands[1]) + summands[2]);
  validate(expectedProduct != (factors[1] * factors[0]) * factors[2]);

  for (const auto& order : Permutations) {
    validateEqualsWithParams3(
      tarch::la::sumInOrderOfMagnitude(summands[order[0]], summands[order[1]], summands[order[2]]),
      expectedSum,
      order[0],
      order[1],
      order[2]
    );
    validateEqualsWithParams3(
      tarch::la::sumInOrderOfMagnitude(-summands[order[0]], -summands[order[1]], -summands[order[2]]),
      -expectedSum,
      order[0],
      order[1],
      order[2]
    );
    validateEqualsWithParams3(
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
    validateEqualsWithParams3(
      tarch::la::productInOrderOfMagnitude(factors[order[0]], factors[order[1]], factors[order[2]]),
      expectedProduct,
      order[0],
      order[1],
      order[2]
    );

    double smallest = orderedValues[order[0]];
    double middle   = orderedValues[order[1]];
    double largest  = orderedValues[order[2]];
    tarch::la::orderByMagnitude(smallest, middle, largest);
    validateEqualsWithParams3(smallest, orderedValues[0], order[0], order[1], order[2]);
    validateEqualsWithParams3(middle, orderedValues[1], order[0], order[1], order[2]);
    validateEqualsWithParams3(largest, orderedValues[2], order[0], order[1], order[2]);
  }
}

/**
 * The sum of the squares of three arguments adds the two small squares first and returns the same
 * bits for every order and every sign of the arguments, also for two largest arguments of equal
 * magnitude.
 */
void
tarch::la::tests::ScalarTest::testEveryOrderAndSignOfTheArgumentsReturnsTheSameSumOfSquares() {
  constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  constexpr int SignPatterns       = 8;

  const double argumentSets[2][3] = {{1.0e8, 1.0, -1.0}, {0.3, -0.7, 0.7}};
  for (const auto& arguments : argumentSets) {
    const double sumInArgumentOrder =
      tarch::la::sumOfSquaresInOrderOfMagnitude(arguments[0], arguments[1], arguments[2]);
    for (const auto& order : Permutations) {
      for (int signs = 0; signs < SignPatterns; signs++) {
        const double first  = (signs & 1) != 0 ? -arguments[order[0]] : arguments[order[0]];
        const double second = (signs & 2) != 0 ? -arguments[order[1]] : arguments[order[1]];
        const double third  = (signs & 4) != 0 ? -arguments[order[2]] : arguments[order[2]];
        validateEqualsWithParams4(
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
  validate(1.0e16 + 2.0 != (1.0e16 + 1.0) + 1.0);
  validateEquals(
    tarch::la::sumOfSquaresInOrderOfMagnitude(argumentSets[0][0], argumentSets[0][1], argumentSets[0][2]),
    1.0e16 + 2.0
  );
}

/**
 * Over a fixed set of arguments, the largest relative error of sumOfSquaresInOrderOfMagnitude()
 * stays below that of sumInOrderOfMagnitude() over the squares that productRoundedOnce() returns.
 */
void
tarch::la::tests::ScalarTest::testTheSumOfSquaresInOrderOfMagnitudeIsMoreExactThanTheSumOfRoundedSquares() {
  constexpr int Samples             = 4096;
  long double   largestFusedError   = 0.0L;
  long double   largestRoundedError = 0.0L;
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
  validateWithParams2(
    largestFusedError < largestRoundedError,
    static_cast<double>(largestFusedError),
    static_cast<double>(largestRoundedError)
  );
}

/**
 * Every order of three arguments returns the bits of the sum in the order of value and of the mean
 * for values whose sum depends on the order. Three equal arguments return their mean bitwise.
 */
void
tarch::la::tests::ScalarTest::testEveryOrderOfTheArgumentsReturnsTheSameSumAndMeanInOrderOfValue() {
  constexpr int Permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

  const double values[3]   = {0.1, 0.7, 0.3};
  const double expectedSum = (values[0] + values[1]) + values[2];
  for (const auto& order : Permutations) {
    validateEqualsWithParams3(
      tarch::la::sumInOrderOfValue(values[order[0]], values[order[1]], values[order[2]]),
      expectedSum,
      order[0],
      order[1],
      order[2]
    );
    validateEqualsWithParams3(
      tarch::la::sumInOrderOfValue(-values[order[0]], -values[order[1]], -values[order[2]]),
      -expectedSum,
      order[0],
      order[1],
      order[2]
    );
  }

  const double expected   = tarch::la::meanInOrderOfValue(values[0], values[1], values[2]);
  const double equalValue = 1.5 + 2.0 * std::numeric_limits<double>::epsilon();
  validateNumericalEquals(expected, (0.1 + 0.7 + 0.3) / 3.0);
  validate(((values[0] + values[1]) + values[2]) / 3.0 != ((values[0] + values[2]) + values[1]) / 3.0);
  validate(((equalValue + equalValue) + equalValue) / 3.0 != equalValue);
  validateEquals(tarch::la::meanInOrderOfValue(equalValue, equalValue, equalValue), equalValue);

  for (const auto& order : Permutations) {
    validateEqualsWithParams3(
      tarch::la::meanInOrderOfValue(values[order[0]], values[order[1]], values[order[2]]),
      expected,
      order[0],
      order[1],
      order[2]
    );
    validateEqualsWithParams3(
      tarch::la::meanInOrderOfValue(-values[order[0]], -values[order[1]], -values[order[2]]),
      -expected,
      order[0],
      order[1],
      order[2]
    );
  }
}

void
tarch::la::tests::ScalarTest::testComparison() {
  const double a   = 1.0;
  const double b   = 2.0;
  const double eps = 1e-14;

  validate(tarch::la::greater(b, a, eps));
  validate(not tarch::la::greater(a, a - eps, eps));
  validate(tarch::la::greater(a, a - 10.0 * eps, eps));

  validate(not tarch::la::greaterEquals(a, b, eps));
  validate(tarch::la::greaterEquals(b, a, eps));
  validate(tarch::la::greaterEquals(a, a, eps));
  validate(tarch::la::greaterEquals(a, a + 0.1 * eps, eps));
  validate(not tarch::la::greaterEquals(a, a + 10 * eps, eps));

  validate(tarch::la::smaller(a, b, eps));
  validate(not tarch::la::smaller(a, a + eps, eps));
  validate(tarch::la::smaller(a, a + 10.0 * eps, eps));

  validate(tarch::la::smallerEquals(a, b, eps));
  validate(tarch::la::smallerEquals(a, a + eps, eps));
  validate(tarch::la::smallerEquals(a + eps, a, eps));
  validate(tarch::la::smallerEquals(a, a + 10.0 * eps, eps));

  validate(not tarch::la::equals(a, b, eps));
  validate(tarch::la::equals(a, a, eps));
  validate(tarch::la::equals(a, a + eps, eps));
  validate(not tarch::la::equals(a, a + 10.0 * eps, eps));
  validate(tarch::la::equals(a, a + 10.0 * eps, 10.0 * eps));
}

void
tarch::la::tests::ScalarTest::testAbs() {
  const double a = -1.0;
  const int    b = -1;

  validateEquals(tarch::la::abs(a), 1.0);
  validateEquals(tarch::la::abs(b), 1);
}

void
tarch::la::tests::ScalarTest::testAdditionalScalarFunctions() {
  validateEquals(tarch::la::sign(2.0), 1);
  validateEquals(tarch::la::sign(-2.0), -1);
  validateEquals(tarch::la::sign(1.0e-10), 0);
  validateEquals(tarch::la::sign(-1.0e-10), 0);
  validateEquals(tarch::la::sign(0.0), 0);
  validateEquals(tarch::la::sign(0.5, 1.0), 0);
  validateEquals(tarch::la::sign(-0.5, 1.0), 0);
  validateEquals(tarch::la::sign(1.5, 1.0), 1);
  validateEquals(tarch::la::sign(-1.5, 1.0), -1);
  validateEquals(tarch::la::round(2.4), 2);
  validateEquals(tarch::la::round(2.6), 3);
  validateEquals(tarch::la::round(2.6F), 3);

  validateEquals(tarch::la::max(1.0, 3.0, 2.0), 3.0);
  validateEquals(tarch::la::min(1.0, 3.0, 2.0), 1.0);
  validateNumericalEquals(tarch::la::pow(4.0, 0.5), 2.0);
}

/**
 * Every order of the entries returns the same fixed-point sum, and negated entries return the
 * negated sum.
 */
void
tarch::la::tests::ScalarTest::testEveryOrderOfTheEntriesReturnsTheSameFixedPointSum() {
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

  const double sum = tarch::la::sumInFixedPoint(values, Size);
  validateEquals(tarch::la::sumInFixedPoint(reversed, Size), sum);
  validateEquals(tarch::la::sumInFixedPoint(interleaved, Size), sum);
  validateEquals(tarch::la::sumInFixedPoint(negated, Size), -sum);
  validateNumericalEqualsWithEps(sum, sumInIndexOrder, 1.0e-10);
  validateEquals(tarch::la::sumInFixedPoint(values, 0), 0.0);

  const double zeros[3] = {0.0, -0.0, 0.0};
  validateEquals(tarch::la::sumInFixedPoint(zeros, 3), 0.0);
  const double infinite[2] = {1.0, std::numeric_limits<double>::infinity()};
  validate(std::isinf(tarch::la::sumInFixedPoint(infinite, 2)));
}
