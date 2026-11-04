// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class TimeSteppingTest;
} // namespace exahype2::tests

class exahype2::tests::TimeSteppingTest final : public tarch::tests::TestCase {
public:
  TimeSteppingTest();
  TimeSteppingTest(const TimeSteppingTest&)            = delete;
  TimeSteppingTest(TimeSteppingTest&&)                 = delete;
  TimeSteppingTest& operator=(const TimeSteppingTest&) = delete;
  TimeSteppingTest& operator=(TimeSteppingTest&&)      = delete;
  ~TimeSteppingTest() override                         = default;

  void run() override;

private:
  void testTheReductionTakesTheLargerPhysicalWaveSpeedBitwise();
  void testTheReductionKeepsAFaultyWaveSpeed();
  void testANewSolverStandsAtTimeZeroWithoutMeshWidths();
  void testTheLimitsReadBackAsSet();
  void testAdvancingTheTimeStampRecordsTheStepSize();
  void testTheMeshWidthsReduceOverEveryRank();
  void testTheAdoptionReportsOnlyTheValuesSetSinceTheLastReduction();
  void testMaxEigenvaluesKeepTheLargestPositiveValue();
  void testStartTimeStepCommitsTheProposalAndClearsTheMaxima();
  void testRankReductionTakesTheMaximaOfAllRanks();
  void testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum();
  void testProposalStopsAtTheTerminalTime();
  void testAFixedRuleKeepsItsSizeAcrossSteps();
};
