// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::particles::assignmentchecks::tests {
  class TestHelpers;
} // namespace peano4::particles::assignmentchecks::tests

class peano4::particles::assignmentchecks::tests::TestHelpers final : public tarch::tests::TestCase {
public:
  TestHelpers();
  void run() override;

private:
  void testTruthTableSearchAndIdKeys();
  void testAddingSweepsToDatabase();
  void testAddingParticleEvents();
  void testRemovingParticleEvents();
  void testParticleMoveEvents();

  void testAddingParticleMovingEvents(int numberOfSweeps = 100, int numberOfEventsToKeep = 1000);

  void testParticleWalkSameTreeLevel();
  void testParticleLiftDrop();
  void testParticleWalk();
  void testPeriodicBoundaryConditions();
  void testPeriodicBoundaryConditionsCorner();
  void testSieveSet();
  void testRepeatedVirtualParticleCleanup();
};
