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
  void testDescribeMeshUpdatesReportsTheStepCountAndWallClock();
  void testDescribeRepetitionsCountsEveryPhaseInItsUnit();
  void testAReportInsideARegridCountsTheRegridAgain();
  void testFinishStepSumsTheSolverBytesAndCellUpdates();
  void testFinishStepReducesTheValuesOfTheSolvers();
  void testTheGridInitialisationAppliesOnlyTheVotesOfTheInitialCondition();
  void testTheTimeLoopWritesTheInitialConditionAndTheLastTimeStamp();
  void testOneSnapshotWritesTheInitialConditionAndTheLastTimeStamp();

  struct TimeLoopCalls {
    int timeSteps{0};
    int plots{0};
    int records{0};
    int timeStepsBeforeTheFirstPlot{0};
    int timeStepsBeforeTheLastPlot{0};
    int timeStepsBeforeTheRecord{0};
  };
  static TimeLoopCalls runTimeLoop();
};
