// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExactAccumulatorTest.h"

#include "tarch/la/ExactAccumulator.h"

tarch::la::tests::ExactAccumulatorTest::ExactAccumulatorTest() :
  TestCase("tarch::la::tests::ExactAccumulatorTest") {}

void tarch::la::tests::ExactAccumulatorTest::run() {
  testMethod(testCorrectRounding);
  testMethod(testOrderInvariance);
  testMethod(testNegationSymmetry);
  testMethod(testSubnormalsAndZero);
  testMethod(testOverflow);
  testMethod(testClear);
}

void tarch::la::tests::ExactAccumulatorTest::testCorrectRounding() {
  tarch::la::ExactAccumulator accumulator;

  accumulator.add(1.0e16);
  accumulator.add(1.0);
  accumulator.add(-1.0e16);
  validateNumericalEquals(accumulator.round(), 1.0);

  constexpr double TwoPower53 = 9007199254740992.0;

  // The exact sum 2^53 + 1 lies halfway between representable neighbours and
  // rounds to the even mantissa.
  accumulator.clear();
  accumulator.add(TwoPower53);
  accumulator.add(1.0);
  validateNumericalEquals(accumulator.round(), TwoPower53);

  // Any sticky remainder below the halfway point breaks the tie upwards.
  accumulator.clear();
  accumulator.add(TwoPower53);
  accumulator.add(1.0);
  accumulator.add(std::ldexp(1.0, -60));
  validateNumericalEquals(accumulator.round(), TwoPower53 + 2.0);

  // Values thousands of orders of magnitude apart coexist without absorption.
  // A numerical comparison accepts any tiny value here, so the bits decide.
  accumulator.clear();
  accumulator.add(1.0e300);
  accumulator.add(1.0e-300);
  accumulator.add(-1.0e300);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(1.0e-300));

  // A sum just below a power of two rounds up to it, so the significand carries
  // out of the mantissa and raises the exponent.
  accumulator.clear();
  accumulator.add(TwoPower53 - 1.0);
  accumulator.add(0.75);
  validateNumericalEquals(accumulator.round(), TwoPower53);
}

void tarch::la::tests::ExactAccumulatorTest::testOrderInvariance() {
  const double  values[]       = {1.0e16, -3.75, 1.0, std::ldexp(1.0, -70), -1.0e16, 0.125, 2.0e8, -0.125};
  constexpr int NumberOfValues = 8;
  const int     permutations[3][NumberOfValues] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {7, 6, 5, 4, 3, 2, 1, 0},
    {4, 0, 6, 2, 7, 5, 1, 3},
  };

  tarch::la::ExactAccumulator accumulator;
  std::uint64_t               referenceBits = 0;
  for (int permutation = 0; permutation < 3; permutation++) {
    accumulator.clear();
    for (const int index : permutations[permutation]) {
      accumulator.add(values[index]);
    }
    const auto bits = std::bit_cast<std::uint64_t>(accumulator.round());
    if (permutation == 0) {
      referenceBits = bits;
    } else {
      validateEquals(bits, referenceBits);
    }
  }
}

void tarch::la::tests::ExactAccumulatorTest::testNegationSymmetry() {
  const double values[] = {0.1, -1.0e12, 3.3333333333333333e-5, 7.0, -0.7, 1.0e-200};

  tarch::la::ExactAccumulator positive;
  tarch::la::ExactAccumulator negative;
  for (const double value : values) {
    positive.add(value);
    negative.add(-value);
  }
  validateEquals(std::bit_cast<std::uint64_t>(negative.round()), std::bit_cast<std::uint64_t>(-positive.round()));
}

void tarch::la::tests::ExactAccumulatorTest::testSubnormalsAndZero() {
  constexpr double MinimumSubnormal       = 4.9406564584124654e-324;
  constexpr double ThreeMinimumSubnormals = 3.0 * MinimumSubnormal;
  constexpr double LargestSubnormal       = 2.225073858507201e-308;
  constexpr double SmallestNormal         = 2.2250738585072014e-308;

  // Every subnormal lies within the tolerance of a numerical comparison, so
  // these results are compared bit by bit.
  tarch::la::ExactAccumulator accumulator;
  accumulator.add(MinimumSubnormal);
  accumulator.add(MinimumSubnormal);
  accumulator.add(MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.round()),
    std::bit_cast<std::uint64_t>(ThreeMinimumSubnormals)
  );

  accumulator.clear();
  accumulator.add(-MinimumSubnormal);
  accumulator.add(-MinimumSubnormal);
  accumulator.add(-MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.round()),
    std::bit_cast<std::uint64_t>(-ThreeMinimumSubnormals)
  );

  // The largest subnormal and the smallest normal sit on either side of the
  // boundary between the two exponent encodings.
  accumulator.clear();
  accumulator.add(LargestSubnormal);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(LargestSubnormal));
  accumulator.add(MinimumSubnormal);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(SmallestNormal));

  accumulator.clear();
  validateNumericalEquals(accumulator.round(), 0.0);
  validate(not std::signbit(accumulator.round()));
  accumulator.add(0.0);
  accumulator.add(-0.0);
  validateNumericalEquals(accumulator.round(), 0.0);
  validate(not std::signbit(accumulator.round()));

  accumulator.add(2.5);
  accumulator.add(-2.5);
  validateNumericalEquals(accumulator.round(), 0.0);
  validate(not std::signbit(accumulator.round()));
}

void tarch::la::tests::ExactAccumulatorTest::testOverflow() {
  constexpr double MaximumFinite = std::numeric_limits<double>::max();
  constexpr double Infinity      = std::numeric_limits<double>::infinity();

  tarch::la::ExactAccumulator accumulator;
  accumulator.add(MaximumFinite);
  accumulator.add(std::ldexp(1.0, 969));
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(MaximumFinite));

  // A second half-ulp reaches the overflow tie, which rounds away from the
  // maximum finite value because its significand is odd.
  accumulator.add(std::ldexp(1.0, 969));
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(Infinity));

  accumulator.clear();
  accumulator.add(MaximumFinite);
  accumulator.add(MaximumFinite);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(Infinity));

  accumulator.clear();
  constexpr int NumberOfLargeValues = 1 << 16;
  for (int value = 0; value < NumberOfLargeValues; value++) {
    accumulator.add(MaximumFinite);
  }
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(Infinity));

  accumulator.clear();
  accumulator.add(-MaximumFinite);
  accumulator.add(-MaximumFinite);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(-Infinity));
}

void tarch::la::tests::ExactAccumulatorTest::testClear() {
  tarch::la::ExactAccumulator accumulator;
  accumulator.add(123.456);
  accumulator.clear();
  accumulator.add(-1.5);
  validateNumericalEquals(accumulator.round(), -1.5);
}
