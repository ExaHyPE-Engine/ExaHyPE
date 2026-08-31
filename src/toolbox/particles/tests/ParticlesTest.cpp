// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticlesTest.h"

#include "toolbox/particles/particles.h"

toolbox::particles::tests::ParticlesTest::ParticlesTest() :
  TestCase("toolbox::particles::tests::ParticlesTest") {}

void toolbox::particles::tests::ParticlesTest::run() {
  testMethod(testPeriodicImageTowards);
  testMethod(testApplyPeriodicBoundaryConditions);
}

/** The image of a position lies within half the domain width of the reference on every periodic axis. */
void toolbox::particles::tests::ParticlesTest::testPeriodicImageTowards() {
  const tarch::la::Vector<DIMENSIONS, double> domainWidth(2.0);
  const tarch::la::Vector<DIMENSIONS, double> lowerWall(0.0);
  const tarch::la::Vector<DIMENSIONS, double> upperWall(2.0);
  const tarch::la::Vector<DIMENSIONS, double> nearUpperWall(1.9);
  const tarch::la::Vector<DIMENSIONS, double> nearLowerWall(0.1);
  std::bitset<DIMENSIONS>                     firstAxisPeriodic;
  firstAxisPeriodic[0] = true;

  const auto unchanged =
    toolbox::particles::periodicImageTowards(nearUpperWall, lowerWall, domainWidth, std::bitset<DIMENSIONS>());
  validateNumericalEquals(unchanged(0), 1.9);

  const auto belowLowerWall =
    toolbox::particles::periodicImageTowards(nearUpperWall, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(belowLowerWall(0), -0.1);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(belowLowerWall(axis), 1.9);
  }

  const auto aboveUpperWall =
    toolbox::particles::periodicImageTowards(nearLowerWall, upperWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(aboveUpperWall(0), 2.1);

  const auto nearReference =
    toolbox::particles::periodicImageTowards(nearLowerWall, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(nearReference(0), 0.1);

  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  const auto                                  halfWidthAway =
    toolbox::particles::periodicImageTowards(centre, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(halfWidthAway(0), 1.0);

  // The test driver initialises a domain without a periodic axis.
  validateFalse(toolbox::particles::domainHasPeriodicAxes());
  const auto withinTheDomain = toolbox::particles::periodicImageTowards(nearUpperWall, lowerWall);
  validateNumericalEquals(withinTheDomain(0), 1.9);

  peano4::datamanagement::CellMarker cell{};
  cell.h                 = tarch::la::Vector<DIMENSIONS, double>(0.5);
  cell.centre            = tarch::la::Vector<DIMENSIONS, double>(0.25);
  const auto towardsCell = toolbox::particles::periodicImageTowardsCell(nearUpperWall, cell);
  validateNumericalEquals(towardsCell(0), 1.9);
}

/** A position outside the domain moves back by one domain width on a periodic axis only. */
void toolbox::particles::tests::ParticlesTest::testApplyPeriodicBoundaryConditions() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(-1.0);
  const tarch::la::Vector<DIMENSIONS, double> domainSize(2.0);
  const tarch::la::Vector<DIMENSIONS, double> beyondUpperWall(1.25);
  const tarch::la::Vector<DIMENSIONS, double> beyondLowerWall(-1.25);
  std::bitset<DIMENSIONS>                     firstAxisPeriodic;
  firstAxisPeriodic[0] = true;

  const auto wrappedDown =
    toolbox::particles::applyPeriodicBoundaryConditions(beyondUpperWall, domainOffset, domainSize, firstAxisPeriodic);
  validateNumericalEquals(wrappedDown(0), -0.75);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(wrappedDown(axis), 1.25);
  }

  const auto wrappedUp =
    toolbox::particles::applyPeriodicBoundaryConditions(beyondLowerWall, domainOffset, domainSize, firstAxisPeriodic);
  validateNumericalEquals(wrappedUp(0), 0.75);

  const auto inside = toolbox::particles::applyPeriodicBoundaryConditions(
    tarch::la::Vector<DIMENSIONS, double>(0.5),
    domainOffset,
    domainSize,
    firstAxisPeriodic
  );
  validateNumericalEquals(inside(0), 0.5);
}
