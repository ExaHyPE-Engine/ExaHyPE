// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonNegativeReductionTest.h"

#include "exahype2/NonNegativeReduction.h"

namespace {
  // A NaN is an invalid observation too, but every ordered comparison against one
  // raises the invalid-operation exception, and the test runner arms that trap. The
  // negative and the infinity below reach the same branch without raising anything.
  constexpr double NegativeObservation = -1.25;
  constexpr double InfiniteObservation = std::numeric_limits<double>::infinity();

  constexpr std::array PhysicalValues{-0.0, 0.0, std::numeric_limits<double>::denorm_min(), 0.5, 1.0, 2.0};

  // The reductions are function templates, so each needs a callable of its own to
  // be passed around.
  constexpr auto Minimum = [](const double currentValue, const double candidateValue) {
    return exahype2::NonNegativeReduction::minimum(currentValue, candidateValue);
  };
  constexpr auto Maximum = [](const double currentValue, const double candidateValue) {
    return exahype2::NonNegativeReduction::maximum(currentValue, candidateValue);
  };
  constexpr auto Sum = [](const double currentValue, const double candidateValue) {
    return exahype2::NonNegativeReduction::sum(currentValue, candidateValue);
  };

  std::uint64_t bits(const double value) { return std::bit_cast<std::uint64_t>(value); }
} // namespace

exahype2::tests::NonNegativeReductionTest::NonNegativeReductionTest() :
  tarch::tests::TestCase("exahype2::tests::NonNegativeReductionTest") {}

void exahype2::tests::NonNegativeReductionTest::run() {
  testMethod(testPhysicalValuesReduceLikeTheStandardOperations);
  testMethod(testInvalidObservationsSurviveFurtherReductions);
}

void exahype2::tests::NonNegativeReductionTest::testPhysicalValuesReduceLikeTheStandardOperations() {
  for (const auto currentValue : PhysicalValues) {
    for (const auto candidateValue : PhysicalValues) {
      // The comparison is bitwise, so the signed zeroes have to agree as well.
      validateEquals(
        bits(exahype2::NonNegativeReduction::minimum(currentValue, candidateValue)),
        bits(std::min(currentValue, candidateValue))
      );
      validateEquals(
        bits(exahype2::NonNegativeReduction::maximum(currentValue, candidateValue)),
        bits(std::max(currentValue, candidateValue))
      );
      validateEquals(
        bits(exahype2::NonNegativeReduction::sum(currentValue, candidateValue)),
        bits(currentValue + candidateValue)
      );
    }
  }
}

void exahype2::tests::NonNegativeReductionTest::testInvalidObservationsSurviveFurtherReductions() {
  // Each reduction is checked in both argument positions, and once more on its own
  // result, because the invalid observation has to survive every later reduction.
  const auto survivesEveryPosition = [](const auto reduction, const double invalidObservation) {
    const auto asCandidate = reduction(1.0, invalidObservation);
    const auto asCurrent   = reduction(invalidObservation, 1.0);
    return bits(asCandidate) == bits(invalidObservation) and bits(asCurrent) == bits(invalidObservation)
      and bits(reduction(asCandidate, 2.0)) == bits(invalidObservation);
  };

  for (const auto invalidObservation : {NegativeObservation, InfiniteObservation}) {
    validateWithParams1(survivesEveryPosition(Minimum, invalidObservation), invalidObservation);
    validateWithParams1(survivesEveryPosition(Maximum, invalidObservation), invalidObservation);
    validateWithParams1(survivesEveryPosition(Sum, invalidObservation), invalidObservation);
  }
}
