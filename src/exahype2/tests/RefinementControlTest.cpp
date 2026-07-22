// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlTest.h"

#include "exahype2/RefinementControl.h"
#include "peano4/Globals.h"

namespace {
  tarch::la::Vector<DIMENSIONS, double> uniformVector(double value) {
    return tarch::la::Vector<DIMENSIONS, double>(value);
  }

  std::string describeEventCount(int numberOfEvents) {
    return "RefinementControl{#Events=" + std::to_string(numberOfEvents) + "}";
  }
} // namespace

exahype2::tests::RefinementControlTest::RefinementControlTest() :
  TestCase("exahype2::tests::RefinementControlTest") {}

void exahype2::tests::RefinementControlTest::run() {
  testMethod(testDefaultRefinementCommandIsErase);
  testMethod(testRefineAndEraseCommandsAreCollected);
  testMethod(testKeepCommandIsNotCollected);
  testMethod(testClearDropsCollectedCommands);
}

// Coarsening is the lowest-priority command, so a criterion that says nothing lets the
// grid coarsen and any other solver's refine request still wins the consensus.
void exahype2::tests::RefinementControlTest::testDefaultRefinementCommandIsErase() {
  validateTrue(exahype2::getDefaultRefinementCommand() == exahype2::RefinementCommand::Erase);
}

void exahype2::tests::RefinementControlTest::testRefineAndEraseCommandsAreCollected() {
  exahype2::RefinementControl control(0);
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Refine);
  validateEquals(control.toString(), describeEventCount(1));

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Erase);
  validateEquals(control.toString(), describeEventCount(2));
}

// A keep command carries no grid change, so it must not enter the request set: an event
// per untouched cell would swamp the merge and the exchange.
void exahype2::tests::RefinementControlTest::testKeepCommandIsNotCollected() {
  exahype2::RefinementControl control(0);

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Keep);
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Refine);
  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Keep);
  validateEquals(control.toString(), describeEventCount(1));
}

void exahype2::tests::RefinementControlTest::testClearDropsCollectedCommands() {
  exahype2::RefinementControl control(0);

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Refine);
  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Erase);
  validateEquals(control.toString(), describeEventCount(2));

  control.clear();
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(0.5), uniformVector(0.1), exahype2::RefinementCommand::Refine);
  validateEquals(control.toString(), describeEventCount(1));
}
