// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/particles/assignmentchecks/tests/TestParticle.h"
#include "tarch/la/Vector.h"
#include "tarch/tests/TestCase.h"

namespace peano4::particles::assignmentchecks::tests {
  class TestHelpers;

  namespace internal {
    bool liftParticle(
      const tarch::la::Vector<DIMENSIONS, double> particleX,
      const tarch::la::Vector<DIMENSIONS, double> vertexX,
      const tarch::la::Vector<DIMENSIONS, double> vertexH,
      const int                                   depth
    );

    bool dropParticle(
      const tarch::la::Vector<DIMENSIONS, double> particleX,
      const tarch::la::Vector<DIMENSIONS, double> vertexH,
      const int                                   depth,
      const int                                   maxVertexDepth
    );

    int findVertexInd(double x, double vertexH);

    tarch::la::Vector<DIMENSIONS, double> findVertexX(
      const tarch::la::Vector<DIMENSIONS, double> x,
      const tarch::la::Vector<DIMENSIONS, double> vertexH
    );

    TestParticle* mirrorParticleAlongPeriodicBoundary(
      TestParticle*                         original,
      bool                                  mirrorX,
      bool                                  mirrorY,
      bool                                  mirrorZ,
      tarch::la::Vector<DIMENSIONS, double> boundary
    );
  } // namespace internal
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
