// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "runner/PhaseNames.h"
#include "runner/phases/DynamicAMR.h"

runner::tests::DynamicAMRTest::DynamicAMRTest() :
  TestCase("runner::tests::DynamicAMRTest") {}

void runner::tests::DynamicAMRTest::run() {
  testMethod(testDescribeReportsOneCountWhenTheLeafCountIsUnchanged);
  testMethod(testDescribeReportsBothCountsWhenTheGridChanged);
  testMethod(testReportThresholdStaysBelowTheDrainBound);
}

// A drain that realises no topology change still reports its sweeps, and a line repeating
// the same count twice would read as a change that did not happen.
void runner::tests::DynamicAMRTest::testDescribeReportsOneCountWhenTheLeafCountIsUnchanged() {
  runner::DynamicAMR::Activity activity;
  activity.sweeps       = 4;
  activity.wallClock    = 0.5;
  activity.leavesBefore = 5121;
  activity.leavesAfter  = 5121;
  activity.minLevel     = 3;
  activity.maxLevel     = 4;

  const std::string line = runner::DynamicAMR::describe(activity);

  validateTrue(line.find(runner::PhaseName::DynamicAMR) != std::string::npos);
  validateTrue(line.find("→") == std::string::npos);
  validateTrue(line.find("3 .. 4") != std::string::npos);
}

void runner::tests::DynamicAMRTest::testDescribeReportsBothCountsWhenTheGridChanged() {
  runner::DynamicAMR::Activity activity;
  activity.sweeps       = 8;
  activity.wallClock    = 1.25;
  activity.leavesBefore = 81;
  activity.leavesAfter  = 1409;
  activity.minLevel     = 2;
  activity.maxLevel     = 5;

  const std::string line = runner::DynamicAMR::describe(activity);

  validateTrue(line.find(runner::PhaseName::DynamicAMR) != std::string::npos);
  validateTrue(line.find("→") != std::string::npos);
  validateTrue(line.find("2 .. 5") != std::string::npos);
}

// A drain has to write at least one progress line before it hits the abort, which is what
// separates a slow drain from a hang in the terminal output.
void runner::tests::DynamicAMRTest::testReportThresholdStaysBelowTheDrainBound() {
  validateTrue(runner::DynamicAMR::SweepsPerReport < runner::DynamicAMR::MaxSweeps);
  validateTrue(runner::DynamicAMR::MaxSweeps % runner::DynamicAMR::SweepsPerReport == 0);
}
