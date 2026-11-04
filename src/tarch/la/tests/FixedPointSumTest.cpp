// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FixedPointSumTest.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/FixedPointSum.h"

namespace {
  constexpr int NumberOfTerms = 8;
  constexpr int Divisor       = 27;

  /**
   * Returns the term index of the sum entry. The terms span 24 binary orders of magnitude and both
   * signs.
   */
  GPU_CALLABLE_METHOD double getTermOf(const int entry, const int index) {
    const double magnitude = std::ldexp(1.0 + 0.1 * index + 0.01 * entry, -3 * index);
    return index % 2 == 0 ? magnitude : -magnitude;
  }

  GPU_CALLABLE_METHOD tarch::la::FixedPointSum<NumberOfTerms> getFixedPointSumOf(const int entry) {
    tarch::la::FixedPointSum<NumberOfTerms> sum;
    sum.clear(2.0);
    for (int index = 0; index < NumberOfTerms; index++) {
      sum.add(getTermOf(entry, index));
    }
    return sum;
  }

  struct CountMismatch final {
    const std::uint64_t* expected{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const auto sum = getFixedPointSumOf(entry);
      if (std::bit_cast<std::uint64_t>(sum.round()) != expected[2 * entry]) {
        accumulator++;
      }
      if (std::bit_cast<std::uint64_t>(sum.roundQuotient<Divisor>()) != expected[2 * entry + 1]) {
        accumulator++;
      }
    }
  };

  int getNumberOfDeviceMismatchesOf(const std::vector<std::uint64_t>& expectedOnHost) {
    tarch::accelerator::Buffer<std::uint64_t> expectedBits(tarch::MemoryLocation::Device);
    expectedBits.setValues(expectedOnHost);
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(static_cast<int>(expectedOnHost.size() / 2)),
      CountMismatch{expectedBits.getData()},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }
} // namespace

tarch::la::tests::FixedPointSumTest::FixedPointSumTest() :
  TestCase("tarch::la::tests::FixedPointSumTest") {}

void
tarch::la::tests::FixedPointSumTest::run() {
  testMethod(testEveryOrderOfTheTermsReturnsTheSameBits);
  testMethod(testNegatedTermsReturnTheNegatedSum);
  testMethod(testTermsOnTheGridReturnTheirExactSum);
  testMethod(testOneTermReturnsItsValue);
  testMethod(testQuotientsRoundToTheNearestDouble);
  testMethod(testDeviceSumsLikeTheHost);
}

/**
 * Every order of the terms returns the same bits.
 */
void
tarch::la::tests::FixedPointSumTest::testEveryOrderOfTheTermsReturnsTheSameBits() {
  const double values[NumberOfTerms] = {0.7, -0.3333333333333333, 1.0e-9, -0x1p-40, 0.125, 0.999, -0.5, 3.0e-17};
  const int    permutations[3][NumberOfTerms] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {7, 6, 5, 4, 3, 2, 1, 0},
    {4, 0, 6, 2, 7, 5, 1, 3},
  };
  tarch::la::FixedPointSum<NumberOfTerms> sum;
  std::uint64_t                           referenceBits = 0;
  for (int permutation = 0; permutation < 3; permutation++) {
    sum.clear(1.0);
    for (const int index : permutations[permutation]) {
      sum.add(values[index]);
    }
    const auto bits = std::bit_cast<std::uint64_t>(sum.round());
    if (permutation == 0) {
      referenceBits = bits;
    } else {
      validateEqualsWithParams1(bits, referenceBits, permutation);
    }
  }
}

/**
 * Negated terms return the negated sum bitwise.
 */
void
tarch::la::tests::FixedPointSumTest::testNegatedTermsReturnTheNegatedSum() {
  for (int entry = 0; entry < 16; entry++) {
    tarch::la::FixedPointSum<NumberOfTerms> positive;
    tarch::la::FixedPointSum<NumberOfTerms> negative;
    positive.clear(2.0);
    negative.clear(2.0);
    for (int index = 0; index < NumberOfTerms; index++) {
      positive.add(getTermOf(entry, index));
      negative.add(-getTermOf(entry, index));
    }
    validateEqualsWithParams1(
      std::bit_cast<std::uint64_t>(negative.round()),
      std::bit_cast<std::uint64_t>(-positive.round()),
      entry
    );
  }
}

/**
 * Terms that keep every bit on the grid return their exact sum.
 */
void
tarch::la::tests::FixedPointSumTest::testTermsOnTheGridReturnTheirExactSum() {
  for (int entry = 0; entry < 64; entry++) {
    // Small integer multiples of powers of two sum exactly in any order.
    tarch::la::FixedPointSum<NumberOfTerms> sum;
    double                                  exactSum = 0.0;
    sum.clear(1024.0);
    for (int index = 0; index < NumberOfTerms; index++) {
      const double term = std::ldexp(static_cast<double>((entry + 3 * index) % 17 - 8), -3 * index);
      sum.add(term);
      exactSum += term;
    }
    validateEqualsWithParams1(std::bit_cast<std::uint64_t>(sum.round()), std::bit_cast<std::uint64_t>(exactSum), entry);
  }
}

/**
 * One term returns its value bitwise, and a sum without terms returns zero.
 */
void
tarch::la::tests::FixedPointSumTest::testOneTermReturnsItsValue() {
  const double                            values[] = {1.0, -0.1, 1.0e5 + 0x1p-30, 3.0e-200, -7.5e250};
  tarch::la::FixedPointSum<NumberOfTerms> sum;
  for (const double value : values) {
    sum.clear(std::abs(value));
    sum.add(value);
    validateEqualsWithParams1(std::bit_cast<std::uint64_t>(sum.round()), std::bit_cast<std::uint64_t>(value), value);
  }
  sum.clear(0.0);
  validateEquals(sum.round(), 0.0);
  sum.add(0.0);
  validateEquals(sum.round(), 0.0);
}

/**
 * The quotient of an exact sum returns the exact quotient rounded to the nearest double with ties
 * to even.
 */
void
tarch::la::tests::FixedPointSumTest::testQuotientsRoundToTheNearestDouble() {
  for (int entry = 0; entry < 64; entry++) {
    // Integer multiples of 2^-20 below 2^24 sum exactly in a double. A double division rounds once.
    tarch::la::FixedPointSum<NumberOfTerms> sum;
    double                                  exactSum = 0.0;
    sum.clear(0x1p25);
    for (int index = 0; index < NumberOfTerms; index++) {
      const std::uint64_t hash = static_cast<std::uint64_t>(entry * NumberOfTerms + index + 1) * 0x9E3779B97F4A7C15ULL;
      const auto          multiple = static_cast<std::int64_t>(hash >> 19) - (std::int64_t{1} << 44);
      const double        term     = std::ldexp(static_cast<double>(multiple), -20);
      sum.add(term);
      exactSum += term;
    }
    const auto expectBits = [&](const double quotient, const double expected) {
      validateEqualsWithParams1(std::bit_cast<std::uint64_t>(quotient), std::bit_cast<std::uint64_t>(expected), entry);
    };
    expectBits(sum.roundQuotient<1>(), exactSum);
    expectBits(sum.roundQuotient<3>(), exactSum / 3.0);
    expectBits(sum.roundQuotient<9>(), exactSum / 9.0);
    expectBits(sum.roundQuotient<Divisor>(), exactSum / Divisor);
  }

  // The halves of 2^54 + 2 and of 2^54 + 6 lie in the middle between two doubles.
  const double terms[3][2] = {{0x1p54, 2.0}, {0x1p54, 6.0}, {-0x1p54, -6.0}};
  const double halves[3]   = {0x1p53, 0x1p53 + 4.0, -0x1p53 - 4.0};
  for (int pair = 0; pair < 3; pair++) {
    tarch::la::FixedPointSum<2> sum;
    sum.clear(0x1p54);
    sum.add(terms[pair][0]);
    sum.add(terms[pair][1]);
    validateEqualsWithParams1(sum.roundQuotient<2>(), halves[pair], pair);
  }
}

/**
 * A device returns the bits of the host for every sum and quotient.
 */
void
tarch::la::tests::FixedPointSumTest::testDeviceSumsLikeTheHost() {
  constexpr int              NumberOfSums = 64;
  std::vector<std::uint64_t> hostBits(2 * NumberOfSums);
  for (int entry = 0; entry < NumberOfSums; entry++) {
    const auto sum                                    = getFixedPointSumOf(entry);
    hostBits[static_cast<std::size_t>(2 * entry)]     = std::bit_cast<std::uint64_t>(sum.round());
    hostBits[static_cast<std::size_t>(2 * entry + 1)] = std::bit_cast<std::uint64_t>(sum.roundQuotient<Divisor>());
  }
  validateEquals(getNumberOfDeviceMismatchesOf(hostBits), 0);
}
