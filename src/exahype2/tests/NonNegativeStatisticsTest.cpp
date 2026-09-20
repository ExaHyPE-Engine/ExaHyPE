// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonNegativeStatisticsTest.h"

#include "exahype2/NonNegativeStatistics.h"

namespace {
  // Every ordered comparison against a NaN raises the invalid-operation exception, which the
  // test runner traps, so the tests observe a negative and an infinite value.
  constexpr double NegativeObservation = -1.25;
  constexpr double InfiniteObservation = std::numeric_limits<double>::infinity();

  std::uint64_t bits(const double value) { return std::bit_cast<std::uint64_t>(value); }
} // namespace

exahype2::tests::NonNegativeStatisticsTest::NonNegativeStatisticsTest() :
  tarch::tests::TestCase("exahype2::tests::NonNegativeStatisticsTest") {}

void exahype2::tests::NonNegativeStatisticsTest::run() {
  testMethod(testDefaultStatisticsHoldNoObservation);
  testMethod(testPhysicalObservationsTrackMinimumAndMaximum);
  testMethod(testAnInvalidObservationReplacesTheRangeAndSticks);
  testMethod(testTheNonPhysicalExtremumCarriesTheInvalidObservation);
  testMethod(testInvalidObservationBitsSurviveARoundTrip);
}

void exahype2::tests::NonNegativeStatisticsTest::testDefaultStatisticsHoldNoObservation() {
  const exahype2::NonNegativeStatistics statistics;
  validateEquals(statistics.hasObservation(), false);
  validateEquals(statistics.hasPhysicalValues(), false);
  validateEquals(statistics.hasInvalidObservation(), false);
  validateEquals(statistics.invalidObservationBitsOrIdentity(), std::numeric_limits<std::uint64_t>::max());
}

void exahype2::tests::NonNegativeStatisticsTest::testPhysicalObservationsTrackMinimumAndMaximum() {
  auto statistics = exahype2::NonNegativeStatistics::accumulate({}, 1.0);
  statistics      = exahype2::NonNegativeStatistics::accumulate(statistics, 0.0);

  validateEquals(statistics.hasObservation(), true);
  validateEquals(statistics.hasPhysicalValues(), true);
  validateEquals(statistics.hasInvalidObservation(), false);
  validateNumericalEquals(statistics.minimum, 0.0);
  validateNumericalEquals(statistics.maximum, 1.0);
  // The identity is what a rank without an invalid observation contributes to the
  // reduction, so it must not collide with any observation's bit pattern.
  validateEquals(statistics.invalidObservationBitsOrIdentity(), std::numeric_limits<std::uint64_t>::max());
}

void exahype2::tests::NonNegativeStatisticsTest::testAnInvalidObservationReplacesTheRangeAndSticks() {
  for (const auto invalidObservation : {NegativeObservation, InfiniteObservation}) {
    auto statistics = exahype2::NonNegativeStatistics::accumulate({}, invalidObservation);
    validateEquals(statistics.hasObservation(), true);
    validateEquals(statistics.hasInvalidObservation(), true);
    validateEquals(statistics.hasPhysicalValues(), false);
    validateEquals(bits(statistics.minimum), bits(invalidObservation));
    validateEquals(bits(statistics.maximum), bits(invalidObservation));
    validateEquals(statistics.invalidObservationBitsOrIdentity(), bits(invalidObservation));

    statistics = exahype2::NonNegativeStatistics::accumulate(statistics, 2.0);
    statistics = exahype2::NonNegativeStatistics::accumulate(statistics, 0.5);
    validateEquals(bits(statistics.minimum), bits(invalidObservation));
    validateEquals(bits(statistics.maximum), bits(invalidObservation));

    auto physicalFirst = exahype2::NonNegativeStatistics::accumulate({}, 2.0);
    physicalFirst      = exahype2::NonNegativeStatistics::accumulate(physicalFirst, invalidObservation);
    validateEquals(bits(physicalFirst.minimum), bits(invalidObservation));
    validateEquals(bits(physicalFirst.maximum), bits(invalidObservation));
  }
}

void exahype2::tests::NonNegativeStatisticsTest::testTheNonPhysicalExtremumCarriesTheInvalidObservation() {
  const exahype2::NonNegativeStatistics infiniteMaximum{0.5, InfiniteObservation};
  validateEquals(infiniteMaximum.hasInvalidObservation(), true);
  validateEquals(infiniteMaximum.invalidObservationBitsOrIdentity(), bits(InfiniteObservation));

  const exahype2::NonNegativeStatistics negativeMinimum{NegativeObservation, 0.5};
  validateEquals(negativeMinimum.hasInvalidObservation(), true);
  validateEquals(negativeMinimum.invalidObservationBitsOrIdentity(), bits(NegativeObservation));
}

void exahype2::tests::NonNegativeStatisticsTest::testInvalidObservationBitsSurviveARoundTrip() {
  // The NaN pattern is compared bitwise and never enters a comparison.
  constexpr std::uint64_t QuietNaNBits = 0x7ff8000000000042;
  for (const auto invalidObservationBits : {QuietNaNBits, bits(NegativeObservation), bits(InfiniteObservation)}) {
    const auto restored = exahype2::NonNegativeStatistics::fromInvalidObservationBits(invalidObservationBits);
    validateEquals(bits(restored.minimum), invalidObservationBits);
    validateEquals(bits(restored.maximum), invalidObservationBits);
  }
}
