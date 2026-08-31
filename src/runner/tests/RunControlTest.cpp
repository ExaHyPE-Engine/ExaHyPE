// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunControlTest.h"

#include "runner/RunControl.h"
#include "tarch/mpi/Rank.h"

runner::tests::RunControlTest::RunControlTest() :
  TestCase("runner::tests::RunControlTest") {}

void runner::tests::RunControlTest::testProgramStepBroadcast() {
  auto& runControl = runner::RunControl::getInstance();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runControl.setNextProgramStep(23);
  }
  validate(runControl.continueToRun());
  validateEquals(runControl.getCurrentProgramStep(), 23);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runControl.setNextProgramStep(7);
  }
  validate(runControl.continueToRun());
  validateEquals(runControl.getCurrentProgramStep(), 7);
}

void runner::tests::RunControlTest::testTerminationBroadcast() {
  auto& runControl = runner::RunControl::getInstance();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runControl.setNextProgramStep(runner::RunControl::Terminate);
  }
  validate(not runControl.continueToRun());
  validateEquals(runControl.getCurrentProgramStep(), runner::RunControl::Terminate);
}

void runner::tests::RunControlTest::run() {
  testMethod(testProgramStepBroadcast);
  testMethod(testTerminationBroadcast);
}
