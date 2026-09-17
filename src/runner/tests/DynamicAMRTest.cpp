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
  testMethod(testDescribeReportsTheSweepCountAndWallClock);
  testMethod(testDescribeReportsUtilisation);
  testMethod(testDescribeAppendsTheGridTransition);
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

void runner::tests::DynamicAMRTest::testDescribeReportsTheSweepCountAndWallClock() {
  runner::DynamicAMR::Activity singleSweep;
  singleSweep.regrids   = 1;
  singleSweep.sweeps    = 1;
  singleSweep.wallClock = 0.25;

  const auto singleSweepLines = runner::DynamicAMR::describe(singleSweep);
  validateTrue(singleSweepLines[0].find(runner::PhaseName::DynamicAMR) != std::string::npos);
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

void runner::tests::DynamicAMRTest::testDescribeReportsUtilisation() {
  runner::DynamicAMR::Activity activity;
  activity.threadUtilisation = 73.25;

  const auto lines = runner::DynamicAMR::describe(activity);
  validateTrue(lines[0].find("Utilisation 73.25 %") != std::string::npos);
}

/** The header line is followed by the grid rows the drains changed. */
void runner::tests::DynamicAMRTest::testDescribeAppendsTheGridTransition() {
  runner::DynamicAMR::Activity activity;
  activity.before.entries[runner::GridSnapshot::GridLeaves] = {"81", "Cells 91"};
  activity.after.entries[runner::GridSnapshot::GridLeaves]  = {"633", "Cells 712"};
  activity.before.entries[runner::GridSnapshot::GridLevels] = {"2 .. 4", ""};
  activity.after.entries[runner::GridSnapshot::GridLevels]  = {"2 .. 4", ""};

  const auto lines = runner::DynamicAMR::describe(activity);
  validateEquals(lines.size(), std::size_t{3});
  validateTrue(lines[1].find("Grid") != std::string::npos);
  validateTrue(lines[2].find("Number of Leaves") != std::string::npos);
  validateTrue(lines[2].find("81 → 633  (Cells 712)") != std::string::npos);
}
