// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExactAccumulatorTest.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/ExactAccumulator.h"

namespace {
  /**
   * Returns a rounded quotient whose sum spans many limbs and whose top digit differs per entry.
   * The entry selects one of the divisors 1, 2, 3, 4, 8, 9 and 27.
   */
  GPU_CALLABLE_METHOD double getRoundedQuotientOf(const int entry) {
    tarch::la::ExactAccumulator accumulator;
    accumulator.add(0.25 * (entry + 1));
    accumulator.add(1.0e-3 * entry * entry);
    tarch::la::ExactAccumulator negative;
    negative.add(-3.0e10 * entry);
    accumulator.merge(negative);
    switch (entry % 7) {
    case 0:
      return accumulator.roundQuotient<1>();
    case 1:
      return accumulator.roundQuotient<2>();
    case 2:
      return accumulator.roundQuotient<3>();
    case 3:
      return accumulator.roundQuotient<4>();
    case 4:
      return accumulator.roundQuotient<8>();
    case 5:
      return accumulator.roundQuotient<9>();
    default:
      return accumulator.roundQuotient<27>();
    }
  }

  /**
   * Returns whether Divisor copies of a third divide back to the third bitwise and whether a sum
   * that cancels down to seven divides like the double division of seven.
   */
  template <int Divisor>
  bool dividesLikeTheDoubleDivision() {
    constexpr double            Third = 1.0 / 3.0;
    tarch::la::ExactAccumulator copies;
    for (int copy = 0; copy < Divisor; copy++) {
      copies.add(Third);
    }
    tarch::la::ExactAccumulator seven;
    seven.add(1.0e16);
    seven.add(-1.0e16);
    seven.add(7.0);
    return std::bit_cast<std::uint64_t>(copies.roundQuotient<Divisor>()) == std::bit_cast<std::uint64_t>(Third)
       and std::bit_cast<std::uint64_t>(seven.roundQuotient<Divisor>()) == std::bit_cast<std::uint64_t>(7.0 / Divisor);
  }

  struct CountMismatch final {
    const std::uint64_t* expected{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      if (std::bit_cast<std::uint64_t>(getRoundedQuotientOf(entry)) != expected[entry]) {
        accumulator++;
      }
    }
  };

  int getNumberOfDeviceMismatchesOf(const std::vector<std::uint64_t>& expectedOnHost) {
    tarch::accelerator::Buffer<std::uint64_t> expectedBits(tarch::MemoryLocation::Device);
    expectedBits.setValues(expectedOnHost);
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(static_cast<int>(expectedOnHost.size())),
      CountMismatch{expectedBits.getData()},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }
} // namespace

tarch::la::tests::ExactAccumulatorTest::ExactAccumulatorTest() :
  TestCase("tarch::la::tests::ExactAccumulatorTest") {}

void
tarch::la::tests::ExactAccumulatorTest::run() {
  testMethod(testCorrectRounding);
  testMethod(testCorrectlyRoundedQuotient);
  testMethod(testOrderInvariance);
  testMethod(testMergingPreservesLowBitsAndCancellation);
  testMethod(testNegationSymmetry);
  testMethod(testSubnormalsAndZero);
  testMethod(testOverflow);
  testMethod(testClear);
  testMethod(testDeviceRoundsLikeTheHost);
}

void
tarch::la::tests::ExactAccumulatorTest::testMergingPreservesLowBitsAndCancellation() {
  tarch::la::ExactAccumulator left;
  tarch::la::ExactAccumulator right;
  left.add(1.0);
  left.add(0x1p-53);
  right.add(0x1p-53);
  left.merge(right);
  validateEquals(left.round(), 1.0 + 0x1p-52);

  left.clear();
  right.clear();
  left.add(1.0e300);
  left.add(1.0);
  right.add(-1.0e300);
  right.add(-2.0);
  left.merge(right);
  validateEquals(left.round(), -1.0);
  for (int doubling = 0; doubling < 40; doubling++) {
    left.merge(left);
  }
  validateEquals(left.round(), -0x1p40);
  right.clear();
  right.add(0x1p40);
  left.merge(right);
  validateEquals(left.round(), 0.0);
}

void
tarch::la::tests::ExactAccumulatorTest::testCorrectRounding() {
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

  // Values six hundred orders of magnitude apart add up without absorption. A numerical comparison
  // accepts any tiny value. The test compares the bits.
  accumulator.clear();
  accumulator.add(1.0e300);
  accumulator.add(1.0e-300);
  accumulator.add(-1.0e300);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(1.0e-300));

  // A sum just below a power of two rounds up to it. The rounding overflows the mantissa and raises
  // the exponent.
  accumulator.clear();
  accumulator.add(TwoPower53 - 1.0);
  accumulator.add(0.75);
  validateNumericalEquals(accumulator.round(), TwoPower53);
}

void
tarch::la::tests::ExactAccumulatorTest::testCorrectlyRoundedQuotient() {
  constexpr double TwoPower53 = 9007199254740992.0;

  validate(dividesLikeTheDoubleDivision<1>());
  validate(dividesLikeTheDoubleDivision<2>());
  validate(dividesLikeTheDoubleDivision<3>());
  validate(dividesLikeTheDoubleDivision<4>());
  validate(dividesLikeTheDoubleDivision<8>());
  validate(dividesLikeTheDoubleDivision<9>());
  validate(dividesLikeTheDoubleDivision<27>());

  // A quotient on the tie between two doubles rounds to the even one, and a remainder of the
  // division breaks the tie upwards.
  tarch::la::ExactAccumulator accumulator;
  accumulator.add(3.0 * TwoPower53);
  accumulator.add(3.0);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<3>()),
    std::bit_cast<std::uint64_t>(TwoPower53)
  );
  accumulator.add(1.0);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<3>()),
    std::bit_cast<std::uint64_t>(TwoPower53 + 2.0)
  );

  accumulator.clear();
  accumulator.add(-7.0);
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.roundQuotient<2>()), std::bit_cast<std::uint64_t>(-3.5));

  // A quotient of subnormals rounds to the nearest subnormal with ties to even, and so does a
  // quotient whose integer digits end at the smallest subnormal.
  constexpr double MinimumSubnormal = std::numeric_limits<double>::denorm_min();
  accumulator.clear();
  accumulator.add(3.0 * MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<2>()),
    std::bit_cast<std::uint64_t>(2.0 * MinimumSubnormal)
  );
  accumulator.clear();
  accumulator.add(5.0 * MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<2>()),
    std::bit_cast<std::uint64_t>(2.0 * MinimumSubnormal)
  );
  accumulator.clear();
  accumulator.add(-7.0 * MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<2>()),
    std::bit_cast<std::uint64_t>(-4.0 * MinimumSubnormal)
  );
  accumulator.clear();
  accumulator.add(7.0 * MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<3>()),
    std::bit_cast<std::uint64_t>(2.0 * MinimumSubnormal)
  );
  accumulator.clear();
  accumulator.add(std::ldexp(1.0, -1021));
  accumulator.add(3.0 * MinimumSubnormal);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<2>()),
    std::bit_cast<std::uint64_t>(std::ldexp(1.0, -1022) + 2.0 * MinimumSubnormal)
  );

  // A quotient below the smallest subnormal rounds to zero with the sign of the sum.
  accumulator.clear();
  accumulator.add(-std::numeric_limits<double>::denorm_min());
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.roundQuotient<4>()), std::bit_cast<std::uint64_t>(-0.0));

  accumulator.clear();
  accumulator.add(0.1);
  accumulator.add(0.2);
  validateEquals(
    std::bit_cast<std::uint64_t>(accumulator.roundQuotient<1>()),
    std::bit_cast<std::uint64_t>(accumulator.round())
  );
}

void
tarch::la::tests::ExactAccumulatorTest::testOrderInvariance() {
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

void
tarch::la::tests::ExactAccumulatorTest::testNegationSymmetry() {
  const double values[] = {0.1, -1.0e12, 3.3333333333333333e-5, 7.0, -0.7, 1.0e-200};

  tarch::la::ExactAccumulator positive;
  tarch::la::ExactAccumulator negative;
  for (const double value : values) {
    positive.add(value);
    negative.add(-value);
  }
  validateEquals(std::bit_cast<std::uint64_t>(negative.round()), std::bit_cast<std::uint64_t>(-positive.round()));
}

void
tarch::la::tests::ExactAccumulatorTest::testSubnormalsAndZero() {
  constexpr double MinimumSubnormal       = 4.9406564584124654e-324;
  constexpr double ThreeMinimumSubnormals = 3.0 * MinimumSubnormal;
  constexpr double LargestSubnormal       = 2.225073858507201e-308;
  constexpr double SmallestNormal         = 2.2250738585072014e-308;

  // Every subnormal lies within the tolerance of a numerical comparison. The test compares these
  // results bit by bit.
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

void
tarch::la::tests::ExactAccumulatorTest::testOverflow() {
  constexpr double MaximumFinite = std::numeric_limits<double>::max();
  constexpr double Infinity      = std::numeric_limits<double>::infinity();

  tarch::la::ExactAccumulator accumulator;
  accumulator.add(MaximumFinite);
  accumulator.add(std::ldexp(1.0, 969));
  validateEquals(std::bit_cast<std::uint64_t>(accumulator.round()), std::bit_cast<std::uint64_t>(MaximumFinite));

  // A second half-ulp reaches the overflow tie. The tie rounds away from the maximum finite value.
  // The significand of that value is odd.
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

void
tarch::la::tests::ExactAccumulatorTest::testClear() {
  tarch::la::ExactAccumulator accumulator;
  accumulator.add(123.456);
  accumulator.clear();
  accumulator.add(-1.5);
  validateNumericalEquals(accumulator.round(), -1.5);
}

void
tarch::la::tests::ExactAccumulatorTest::testDeviceRoundsLikeTheHost() {
  constexpr int              NumberOfSums = 64;
  std::vector<std::uint64_t> hostBits(NumberOfSums);
  for (int entry = 0; entry < NumberOfSums; entry++) {
    hostBits[static_cast<std::size_t>(entry)] = std::bit_cast<std::uint64_t>(getRoundedQuotientOf(entry));
  }
  validateEquals(getNumberOfDeviceMismatchesOf(hostBits), 0);
}
