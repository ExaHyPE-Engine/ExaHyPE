// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlTest.h"

#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  // A command names the cell it was raised for, so the coordinates have to be a cell of
  // the lattice: A level-four grid of cells of width 0.1 per axis, and 0.45 is the centre
  // of cell four.
  constexpr int    TestCellsPerAxis = peano4::Power<peano4::sfc::ActiveCurve::SubdivisionFactor, 4>;
  constexpr double TestCellWidth    = 0.1;
  constexpr double TestDomainWidth  = TestCellsPerAxis * TestCellWidth;
  constexpr double TestCellCentre   = 0.45;

  tarch::la::Vector<DIMENSIONS, double> uniformVector(double value) {
    return tarch::la::Vector<DIMENSIONS, double>(value);
  }

  // The service holds the lattice every command is named against, and it outlives a single
  // test, so each test anchors it before raising a command.
  void anchorTestDomain() {
    exahype2::RefinementControlService::getInstance().setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
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
// grid coarsen.
void exahype2::tests::RefinementControlTest::testDefaultRefinementCommandIsErase() {
  validateTrue(exahype2::getDefaultRefinementCommand() == exahype2::RefinementCommand::Erase);
}

void exahype2::tests::RefinementControlTest::testRefineAndEraseCommandsAreCollected() {
  anchorTestDomain();
  exahype2::RefinementControl control(0);
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Refine);
  validateEquals(control.toString(), describeEventCount(1));

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Erase);
  validateEquals(control.toString(), describeEventCount(2));
}

// A keep command carries no grid change, so it does not enter the request set.
void exahype2::tests::RefinementControlTest::testKeepCommandIsNotCollected() {
  anchorTestDomain();
  exahype2::RefinementControl control(0);

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Keep);
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Refine);
  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Keep);
  validateEquals(control.toString(), describeEventCount(1));
}

void exahype2::tests::RefinementControlTest::testClearDropsCollectedCommands() {
  anchorTestDomain();
  exahype2::RefinementControl control(0);

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Refine);
  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Erase);
  validateEquals(control.toString(), describeEventCount(2));

  control.clear();
  validateEquals(control.toString(), describeEventCount(0));

  control.addCommand(uniformVector(TestCellCentre), uniformVector(TestCellWidth), exahype2::RefinementCommand::Refine);
  validateEquals(control.toString(), describeEventCount(1));
}
