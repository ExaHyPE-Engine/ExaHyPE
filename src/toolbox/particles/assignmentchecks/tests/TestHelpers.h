
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"
#include "tarch/la/Vector.h"

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace tests {
        class TestHelpers;

        namespace internal {

          /* bool allVerbose; */

          /**
           * Does this particle need to be lifted? Yes if particleX
           * is not located in the square with size 2 vertexH around
           * the closest vertex with size vertexH, and if we're not
           * at shallowest point in tree (= depth == 0)
           */
          bool liftParticle(
            const tarch::la::Vector<Dimensions, double> particleX,
            const tarch::la::Vector<Dimensions, double> vertexX,
            const tarch::la::Vector<Dimensions, double> vertexH,
            const int depth
          );

          /**
           * Does this particle need to be dropped? Yes if particleX
           * is located in the square with size 2 vertexH around
           * the closest vertex with size vertexH and while
           * depth doesn't exceed maxVertexDepth.
           */
          bool dropParticle(
            const tarch::la::Vector<Dimensions, double> particleX,
            const tarch::la::Vector<Dimensions, double> vertexH,
            const int depth,
            const int maxVertexDepth
          );

          /**
           * Find "index" of vertex with size `vertexH` covering position `x`
           * assuming all vertices start at (0, 0, 0).
           */
          int findVertexInd(double x, double vertexH);

          /**
           * Find position of vertex with size `vertexH` covering position `x`
           * assuming all vertices start at (0, 0, 0).
           */
          tarch::la::Vector<Dimensions, double> findVertexX(
            const tarch::la::Vector<Dimensions, double> x,
            const tarch::la::Vector<Dimensions, double> vertexH
          );
        } // namespace internal
      }   // namespace tests
    }     // namespace assignmentchecks
  }       // namespace particles
}         // namespace toolbox

class toolbox::particles::assignmentchecks::tests::TestHelpers: public tarch::tests::TestCase {

  private:

    /**
     * Test a particle walking from vertex to vertex on the same depth of
     * the tree, i.e. assignments from vertex to vertex without lifts
     * and drops.
     */
    void testParticleWalkSameTreeLevel();

    /**
     * Test a particle being assigned up and down the vertex hierarchy.
     */
    void testParticleLiftDrop();

    /**
     * Test a particle walking from vertex to vertex, where each time a
     * it crosses a vertex boundary, it's assigned to a new vertex via
     * a lift and a subsequent drop.
     */
    void testParticleWalk();

    void testLongParticleWalk();

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

