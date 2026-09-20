// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class NonNegativeStatisticsTest;
} // namespace exahype2::tests

class exahype2::tests::NonNegativeStatisticsTest final : public tarch::tests::TestCase {
public:
  NonNegativeStatisticsTest();
  NonNegativeStatisticsTest(const NonNegativeStatisticsTest&)            = delete;
  NonNegativeStatisticsTest(NonNegativeStatisticsTest&&)                 = delete;
  NonNegativeStatisticsTest& operator=(const NonNegativeStatisticsTest&) = delete;
  NonNegativeStatisticsTest& operator=(NonNegativeStatisticsTest&&)      = delete;
  ~NonNegativeStatisticsTest() override                                  = default;

  void run() override;

private:
  void testDefaultStatisticsHoldNoObservation();
  void testPhysicalObservationsTrackMinimumAndMaximum();
  void testAnInvalidObservationReplacesTheRangeAndSticks();
  void testTheNonPhysicalExtremumCarriesTheInvalidObservation();
  void testInvalidObservationBitsSurviveARoundTrip();
};
