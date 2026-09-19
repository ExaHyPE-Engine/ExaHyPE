// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class ErrorMeasurementTest;
} // namespace exahype2::tests

class exahype2::tests::ErrorMeasurementTest final : public tarch::tests::TestCase {
public:
  ErrorMeasurementTest();
  ErrorMeasurementTest(const ErrorMeasurementTest&)            = delete;
  ErrorMeasurementTest(ErrorMeasurementTest&&)                 = delete;
  ErrorMeasurementTest& operator=(const ErrorMeasurementTest&) = delete;
  ErrorMeasurementTest& operator=(ErrorMeasurementTest&&)      = delete;
  ~ErrorMeasurementTest() override                             = default;

  void run() override;

private:
  void testReductionSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks();
  void testAccumulationWeightsTheMeasuredUnknownsOnly();
  void testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom();
  void testRecordedSolutionErrorReachesTheMeasurement();
};
