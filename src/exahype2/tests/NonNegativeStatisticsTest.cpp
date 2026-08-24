// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonNegativeStatisticsTest.h"

#include "exahype2/NonNegativeStatistics.h"

namespace {
  // A NaN is an invalid observation too, but every ordered comparison against one
  // raises the invalid-operation exception, and the test runner arms that trap. The
  // negative and the infinity below reach the same branch without raising anything.
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

    // Later physical observations must not wash the invalid one out, or a rank
    // would report a healthy range for a step that produced a broken value.
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

void exahype2::tests::NonNegativeStatisticsTest::testInvalidObservationBitsSurviveARoundTrip() {
  // The reduction sends the invalid observation across ranks as a bit pattern, so
  // the pattern that comes back has to rebuild the very same value. A NaN pattern
  // is compared bitwise here and never enters a comparison.
  constexpr std::uint64_t QuietNaNBits = 0x7ff8000000000042;
  for (const auto invalidObservationBits : {QuietNaNBits, bits(NegativeObservation), bits(InfiniteObservation)}) {
    const auto restored = exahype2::NonNegativeStatistics::fromInvalidObservationBits(invalidObservationBits);
    validateEquals(bits(restored.minimum), invalidObservationBits);
    validateEquals(bits(restored.maximum), invalidObservationBits);
  }
}
