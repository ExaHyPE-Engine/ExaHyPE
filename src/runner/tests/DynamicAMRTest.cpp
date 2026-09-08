// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "runner/Format.h"
#include "runner/PhaseNames.h"
#include "runner/RepositoryCallbacks.h"
#include "runner/phases/DynamicAMR.h"

runner::tests::DynamicAMRTest::DynamicAMRTest() :
  TestCase("runner::tests::DynamicAMRTest") {}

void runner::tests::DynamicAMRTest::run() {
  testMethod(testDescribeReportsExplicitUnchangedTransitions);
  testMethod(testDescribeReportsGridTransitions);
  testMethod(testDescribeReportsOneLinePerLoadCategory);
  testMethod(testDescribeDropsTheInterRankLineOnOneRank);
  testMethod(testDescribeReportsBalanceTransitions);
  testMethod(testDescribeReportsTaskLoadAndUtilisation);
  testMethod(testDescribeReportsTheSweepCountAndWallClock);
  testMethod(testADrainNeedsPendingMeshUpdates);
}

void runner::tests::DynamicAMRTest::testADrainNeedsPendingMeshUpdates() {
  runner::RepositoryCallbacks repositories;
  validateFalse(runner::DynamicAMR::resolvePendingMeshUpdates(repositories));

  repositories.hasPendingMeshUpdates = []() { return false; };
  validateFalse(runner::DynamicAMR::resolvePendingMeshUpdates(repositories));

  repositories.hasPendingMeshUpdates = []() { return true; };
  validateTrue(runner::DynamicAMR::resolvePendingMeshUpdates(repositories));
}

void runner::tests::DynamicAMRTest::testDescribeReportsExplicitUnchangedTransitions() {
  runner::DynamicAMR::Activity activity;
  activity.sweeps       = 4;
  activity.wallClock    = 0.5;
  activity.leavesBefore = 5121;
  activity.leavesAfter  = 5121;
  activity.minimumLevel = 3;
  activity.maximumLevel = 4;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateTrue(lines[0].find(runner::PhaseName::DynamicAMR) != std::string::npos);
  validateTrue(lines[1].find("5,121 → 5,121") != std::string::npos);
  validateTrue(lines[1].find("3 .. 4") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsGridTransitions() {
  runner::DynamicAMR::Activity activity;
  activity.sweeps       = 8;
  activity.wallClock    = 1.25;
  activity.leavesBefore = 81;
  activity.leavesAfter  = 1409;
  activity.minimumLevel = 2;
  activity.maximumLevel = 5;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateTrue(lines[0].find(runner::PhaseName::DynamicAMR) != std::string::npos);
  validateTrue(lines[1].find("81 → 1,409") != std::string::npos);
  validateTrue(lines[1].find("2 .. 5") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsOneLinePerLoadCategory() {
  runner::DynamicAMR::Activity activity;
  activity.numberOfRanks = 4;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateEquals(lines.size(), 4);
  validateTrue(lines[2].find("MPI") != std::string::npos);
  validateTrue(lines[2].find("Balance = ") != std::string::npos);
  validateTrue(lines[3].find("OpenMP") != std::string::npos);
  validateTrue(lines[3].find("Tasks = ") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeDropsTheInterRankLineOnOneRank() {
  runner::DynamicAMR::Activity activity;
  activity.numberOfRanks = 1;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateEquals(lines.size(), 3);
  validateTrue(lines[2].find("OpenMP") != std::string::npos);
  validateTrue(lines[2].find("Tasks = ") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsBalanceTransitions() {
  runner::DynamicAMR::Activity activity;
  activity.numberOfRanks          = 4;
  activity.interRankBalanceBefore = 91.5;
  activity.interRankBalanceAfter  = 91.5;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateTrue(lines[2].find("91.50 % → 91.50 %") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsTaskLoadAndUtilisation() {
  runner::DynamicAMR::Activity activity;
  activity.numberOfRanks     = 4;
  activity.subtreeTasksAfter = 226;
  activity.minimumTaskCells  = 7;
  activity.maximumTaskCells  = 486;
  activity.threadUtilisation = 73.25;

  const auto lines = runner::DynamicAMR::describe(activity);

  validateTrue(lines[3].find("0 → 226") != std::string::npos);
  validateTrue(lines[3].find("7 .. 486") != std::string::npos);
  validateTrue(lines[3].find("73.25 %") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsTheSweepCountAndWallClock() {
  runner::DynamicAMR::Activity singleSweep;
  singleSweep.regrids   = 1;
  singleSweep.sweeps    = 1;
  singleSweep.wallClock = 0.25;

  const auto singleSweepLines = runner::DynamicAMR::describe(singleSweep);
  validateTrue(singleSweepLines[0].find("1 Regrid · 1 Sweep") != std::string::npos);
  validateTrue(singleSweepLines[0].find("1 Sweeps") == std::string::npos);
  validateTrue(singleSweepLines[0].find(runner::Format::wallClock(0.25)) != std::string::npos);

  runner::DynamicAMR::Activity severalSweeps;
  severalSweeps.regrids   = 12;
  severalSweeps.sweeps    = 2048;
  severalSweeps.wallClock = 12.5;

  const auto severalSweepLines = runner::DynamicAMR::describe(severalSweeps);
  validateTrue(severalSweepLines[0].find("12 Regrids · 2,048 Sweeps") != std::string::npos);
  validateTrue(severalSweepLines[0].find(runner::Format::wallClock(12.5)) != std::string::npos);
}
