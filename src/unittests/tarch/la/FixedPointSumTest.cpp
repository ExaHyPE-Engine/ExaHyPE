// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/FixedPointSum.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int NumberOfTerms = 8;
  constexpr int Divisor       = 27;

  /**
   * Returns the term index of the sum entry. The terms span 24 binary orders of magnitude and both
   * signs. The fused multiply-adds round every term alike on the host and on a device.
   */
  GPU_CALLABLE_METHOD double getTermOf(const int entry, const int index) {
    const double unscaledMagnitude =
      std::fma(0.01, static_cast<double>(entry), std::fma(0.1, static_cast<double>(index), 1.0));
    const double magnitude = std::ldexp(unscaledMagnitude, -3 * index);
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

UNIT_TEST("Every order of the terms returns the same bits") {
  GIVEN("Eight terms of mixed magnitudes and signs and three orders of them") {
    const double values[NumberOfTerms] = {0.7, -0.3333333333333333, 1.0e-9, -0x1p-40, 0.125, 0.999, -0.5, 3.0e-17};
    const int    permutations[3][NumberOfTerms] = {
      {0, 1, 2, 3, 4, 5, 6, 7},
      {7, 6, 5, 4, 3, 2, 1, 0},
      {4, 0, 6, 2, 7, 5, 1, 3},
    };

    WHEN("A sum adds the terms in every order") {
      std::vector<std::uint64_t>              bits;
      tarch::la::FixedPointSum<NumberOfTerms> sum;
      for (const auto& permutation : permutations) {
        sum.clear(1.0);
        for (const int index : permutation) {
          sum.add(values[index]);
        }
        bits.push_back(std::bit_cast<std::uint64_t>(sum.round()));
      }

      THEN("Every order returns the bits of the first order") {
        for (std::size_t permutation = 1; permutation < bits.size(); permutation++) {
          VALIDATE_EQUALS(bits[permutation], bits[0], permutation);
        }
      }
    }
  }
}

UNIT_TEST("Negated terms return the negated sum to the last bit") {
  GIVEN("Sixteen sets of eight terms that span 24 binary orders of magnitude") {
    constexpr int NumberOfSets = 16;

    WHEN("One sum adds the terms of every set and another sum adds the negated terms") {
      std::vector<std::uint64_t> negatedSums;
      std::vector<std::uint64_t> sumsOfNegatedTerms;
      for (int entry = 0; entry < NumberOfSets; entry++) {
        tarch::la::FixedPointSum<NumberOfTerms> positive;
        tarch::la::FixedPointSum<NumberOfTerms> negative;
        positive.clear(2.0);
        negative.clear(2.0);
        for (int index = 0; index < NumberOfTerms; index++) {
          positive.add(getTermOf(entry, index));
          negative.add(-getTermOf(entry, index));
        }
        negatedSums.push_back(std::bit_cast<std::uint64_t>(-positive.round()));
        sumsOfNegatedTerms.push_back(std::bit_cast<std::uint64_t>(negative.round()));
      }

      THEN("The sum of the negated terms of every set is the negated sum bit for bit") {
        for (std::size_t entry = 0; entry < negatedSums.size(); entry++) {
          VALIDATE_EQUALS(sumsOfNegatedTerms[entry], negatedSums[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("Terms on the grid of the sum return their exact sum") {
  GIVEN("Sixty-four sets of eight small integer multiples of powers of two") {
    constexpr int NumberOfSets = 64;

    WHEN("A sum and a double add the terms of every set") {
      std::vector<std::uint64_t> sums;
      std::vector<std::uint64_t> exactSums;
      for (int entry = 0; entry < NumberOfSets; entry++) {
        // Small integer multiples of powers of two sum exactly in any order.
        tarch::la::FixedPointSum<NumberOfTerms> sum;
        double                                  exactSum = 0.0;
        sum.clear(1024.0);
        for (int index = 0; index < NumberOfTerms; index++) {
          const double term = std::ldexp(static_cast<double>((entry + 3 * index) % 17 - 8), -3 * index);
          sum.add(term);
          exactSum += term;
        }
        sums.push_back(std::bit_cast<std::uint64_t>(sum.round()));
        exactSums.push_back(std::bit_cast<std::uint64_t>(exactSum));
      }

      THEN("The sum of every set is the exact sum bit for bit") {
        for (std::size_t entry = 0; entry < sums.size(); entry++) {
          VALIDATE_EQUALS(sums[entry], exactSums[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("One term returns its value and a sum without terms returns zero") {
  GIVEN("Five values between 3e-200 and 7.5e250 in magnitude") {
    const double values[] = {1.0, -0.1, 1.0e5 + 0x1p-30, 3.0e-200, -7.5e250};

    WHEN("A sum adds every value on its own") {
      std::vector<std::uint64_t>              sums;
      tarch::la::FixedPointSum<NumberOfTerms> sum;
      for (const double value : values) {
        sum.clear(std::abs(value));
        sum.add(value);
        sums.push_back(std::bit_cast<std::uint64_t>(sum.round()));
      }

      THEN("The sum returns every value bit for bit") {
        for (std::size_t entry = 0; entry < sums.size(); entry++) {
          VALIDATE_EQUALS(sums[entry], std::bit_cast<std::uint64_t>(values[entry]), entry);
        }
      }
    }

    WHEN("A sum with the bound zero rounds before and after it adds the term zero") {
      tarch::la::FixedPointSum<NumberOfTerms> sum;
      sum.clear(0.0);
      const double emptySum = sum.round();
      sum.add(0.0);
      const double sumOfZero = sum.round();

      THEN("Both sums are zero") {
        VALIDATE_EQUALS(emptySum, 0.0);
        VALIDATE_EQUALS(sumOfZero, 0.0);
      }
    }
  }
}

UNIT_TEST("The quotient of an exact sum rounds the exact quotient to the nearest double with ties to even") {
  GIVEN("Sixty-four sets of eight integer multiples of 2^-20 below 2^24") {
    constexpr int NumberOfSets = 64;

    WHEN("A sum and a double add the terms of every set and divide by 1, 3, 9 and 27") {
      // Integer multiples of 2^-20 below 2^24 sum exactly in a double. A double division rounds once.
      std::vector<std::uint64_t> quotients;
      std::vector<std::uint64_t> expectedQuotients;
      for (int entry = 0; entry < NumberOfSets; entry++) {
        tarch::la::FixedPointSum<NumberOfTerms> sum;
        double                                  exactSum = 0.0;
        sum.clear(0x1p25);
        for (int index = 0; index < NumberOfTerms; index++) {
          const std::uint64_t hash =
            static_cast<std::uint64_t>(entry * NumberOfTerms + index + 1) * 0x9E3779B97F4A7C15ULL;
          const auto   multiple = static_cast<std::int64_t>(hash >> 19) - (std::int64_t{1} << 44);
          const double term     = std::ldexp(static_cast<double>(multiple), -20);
          sum.add(term);
          exactSum += term;
        }
        quotients.push_back(std::bit_cast<std::uint64_t>(sum.roundQuotient<1>()));
        expectedQuotients.push_back(std::bit_cast<std::uint64_t>(exactSum));
        quotients.push_back(std::bit_cast<std::uint64_t>(sum.roundQuotient<3>()));
        expectedQuotients.push_back(std::bit_cast<std::uint64_t>(exactSum / 3.0));
        quotients.push_back(std::bit_cast<std::uint64_t>(sum.roundQuotient<9>()));
        expectedQuotients.push_back(std::bit_cast<std::uint64_t>(exactSum / 9.0));
        quotients.push_back(std::bit_cast<std::uint64_t>(sum.roundQuotient<Divisor>()));
        expectedQuotients.push_back(std::bit_cast<std::uint64_t>(exactSum / Divisor));
      }

      THEN("Every quotient is the double division of the exact sum bit for bit") {
        for (std::size_t entry = 0; entry < quotients.size(); entry++) {
          VALIDATE_EQUALS(quotients[entry], expectedQuotients[entry], entry);
        }
      }
    }

    WHEN("A sum halves 2^54 + 2, 2^54 + 6 and their negative counterpart") {
      // The halves of 2^54 + 2 and of 2^54 + 6 lie in the middle between two doubles.
      const double        terms[3][2] = {{0x1p54, 2.0}, {0x1p54, 6.0}, {-0x1p54, -6.0}};
      std::vector<double> halves;
      for (const auto& pair : terms) {
        tarch::la::FixedPointSum<2> sum;
        sum.clear(0x1p54);
        sum.add(pair[0]);
        sum.add(pair[1]);
        halves.push_back(sum.roundQuotient<2>());
      }

      THEN("Every half rounds to the even neighbour") {
        VALIDATE_EQUALS(halves[0], 0x1p53);
        VALIDATE_EQUALS(halves[1], 0x1p53 + 4.0);
        VALIDATE_EQUALS(halves[2], -0x1p53 - 4.0);
      }
    }
  }
}

UNIT_TEST("A dispatch returns the bits of the host for every sum and quotient") {
  GIVEN("The bits of sixty-four sums and of their quotients by 27 on the host") {
    constexpr int              NumberOfSums = 64;
    std::vector<std::uint64_t> hostBits(2 * NumberOfSums);
    for (int entry = 0; entry < NumberOfSums; entry++) {
      const auto sum                                    = getFixedPointSumOf(entry);
      hostBits[static_cast<std::size_t>(2 * entry)]     = std::bit_cast<std::uint64_t>(sum.round());
      hostBits[static_cast<std::size_t>(2 * entry + 1)] = std::bit_cast<std::uint64_t>(sum.roundQuotient<Divisor>());
    }

    WHEN("A dispatch computes every sum and quotient and counts the values that differ from the host") {
      const int mismatches = getNumberOfDeviceMismatchesOf(hostBits);

      THEN("Every value of the dispatch has the bits of the host") { VALIDATE_EQUALS(mismatches, 0); }
    }
  }
}
