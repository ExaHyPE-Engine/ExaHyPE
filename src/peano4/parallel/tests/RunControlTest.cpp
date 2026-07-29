// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunControlTest.h"

#include "peano4/parallel/RunControl.h"
#include "tarch/mpi/Rank.h"

peano4::parallel::tests::RunControlTest::RunControlTest() :
  TestCase("peano4::parallel::tests::RunControlTest") {}

/**
 * Every rank enters continueToRun() in lockstep, as the whole collection runs
 * on every rank in the same order, so the collective inside is matched.
 */
void peano4::parallel::tests::RunControlTest::testProgramStepBroadcast() {
  auto& runControl = peano4::parallel::RunControl::getInstance();

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

void peano4::parallel::tests::RunControlTest::testTerminationBroadcast() {
  auto& runControl = peano4::parallel::RunControl::getInstance();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runControl.setNextProgramStep(peano4::parallel::RunControl::Terminate);
  }
  validate(not runControl.continueToRun());
  validateEquals(runControl.getCurrentProgramStep(), peano4::parallel::RunControl::Terminate);
}

void peano4::parallel::tests::RunControlTest::run() {
  testMethod(testProgramStepBroadcast);
  testMethod(testTerminationBroadcast);
}
