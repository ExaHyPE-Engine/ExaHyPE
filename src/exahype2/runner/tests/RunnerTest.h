// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::runner::tests {
  class RunnerTest;
} // namespace exahype2::runner::tests

class exahype2::runner::tests::RunnerTest final : public tarch::tests::TestCase {
public:
  RunnerTest();
  RunnerTest(const RunnerTest&)            = delete;
  RunnerTest(RunnerTest&&)                 = delete;
  RunnerTest& operator=(const RunnerTest&) = delete;
  RunnerTest& operator=(RunnerTest&&)      = delete;
  ~RunnerTest() override                   = default;

  void run() override;

private:
  void testTheMasterStepReachesEveryRank();
  void testTheTerminationStepReachesEveryRank();
  void testDescribeMeshUpdatesReportsTheSweepCountAndWallClock();
  void testDescribeRepetitionsCountsEveryPhaseInItsUnit();
  void testFinishStepSumsTheSolverBytesAndCellUpdates();
  void testTheGridInitialisationAppliesOnlyTheVotesOfTheInitialCondition();
};
