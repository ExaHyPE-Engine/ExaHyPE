
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace tests {
        class TestHelpers;
      } // namespace assignmentchecks
    }   // namespace tests
  }     // namespace particles
}       // namespace toolbox

class toolbox::particles::assignmentchecks::tests::TestHelpers: public tarch::tests::TestCase {

  private:

    /**
     * Test a particle walking from vertex to vertex on the same depth of
     * the tree, i.e. assignments from vertex to vertex without lifts
     * and drops.
     */
    void testParticleWalkSameTreeLevel();

    /**
     * Particles are identified in the events database using their unique ID.
     * However, if we have periodic boundary conditions, we may find that
     * particles with the same ID exist several times on the same tree, on
     * either side of the boundary, provided the tree spans both edges of the
     * domain. So check that we indeed find the correct ancestors. */
    void testPeriodicBoundaryConditions();

  public:
    TestHelpers();
    virtual void run() override;
};

