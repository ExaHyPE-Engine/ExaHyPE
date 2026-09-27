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
  ErrorMeasurementTest(const ErrorMeasurementTest& other)            = delete;
  ErrorMeasurementTest(ErrorMeasurementTest&& other)                 = delete;
  ErrorMeasurementTest& operator=(const ErrorMeasurementTest& other) = delete;
  ErrorMeasurementTest& operator=(ErrorMeasurementTest&& other)      = delete;
  ~ErrorMeasurementTest() override                                   = default;

  void run() override;

private:
  void testRecordSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks();
  void testDeviceSumsMatchTheHostSumsWhateverTheCellOrder();
  void testRecordIsExactAcrossRankPartitions();
  void testAccumulationWeightsTheMeasuredUnknownsOnly();
  void testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom();
  void testEveryRankFindsASnapshotDueForTheNewestTimeStampAndAfterTheTimeDelta();
  void testTheOwnedBufferSumsTheErrorsOfEveryCell();
};
