// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class AdaptiveTimeSteppingTest;
} // namespace exahype2::tests

class exahype2::tests::AdaptiveTimeSteppingTest final : public tarch::tests::TestCase {
public:
  AdaptiveTimeSteppingTest();
  AdaptiveTimeSteppingTest(const AdaptiveTimeSteppingTest&)            = delete;
  AdaptiveTimeSteppingTest(AdaptiveTimeSteppingTest&&)                 = delete;
  AdaptiveTimeSteppingTest& operator=(const AdaptiveTimeSteppingTest&) = delete;
  AdaptiveTimeSteppingTest& operator=(AdaptiveTimeSteppingTest&&)      = delete;
  ~AdaptiveTimeSteppingTest() override                                 = default;

  void run() override;

private:
  void testMaxEigenvaluesKeepTheLargestPositiveValue();
  void testStartTimeStepCommitsTheProposalAndClearsTheMaxima();
  void testRankReductionTakesTheMaximaOfAllRanks();
  void testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum();
  void testProposalStopsAtTheTerminalTime();
};
