// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace tests {

        class TestHelpers;

        namespace internal {

          // TODO: clean
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
            const int                                   depth
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
            const int                                   depth,
            const int                                   maxVertexDepth
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
} // namespace toolbox


class toolbox::particles::assignmentchecks::tests::TestHelpers:
  public tarch::tests::TestCase {

  /* TODO: Verbose parameters */
private:
  /**
   * Test whether ParticleIdentifier and ParticleSearchIdentifier
   * work as intended in a map for a fuzzy search.
   */
  void testTruthTableSearchAndIDKeys(bool verbose = false);


  /**
   * Make sure that adding mesh sweeps to the database works.
   */
  void testAddingSweepsToDatabase(bool verbose = false);


  /**
   * Test the adding of particle events to the database.
   * This also serves as a unit test for all possible events: I let
   * the particles run through some meaningless mesh sweeps, and add
   * all the event types.
   *
   * Here, I only use static particles, so the identifier remains
   * the same all the time. Switching particle identifiers will be
   * done in a separate test.
   * We're also adding events without them having any meaning. Proper
   * event tracing including consistency checks will also be done later.
   */
  void testAddingParticleEvents(bool verbose = false);


  /**
   * Test the adding of particle events to the database with a moving
   * particle. The catch is twofold: Firstly, the fuzzy search needs to work.
   * Secondly, a moving particle will eventually need to change its identifier
   * in the database. This tests both of these cases, but not the event
   * deletion when the database becomes too large.
   *
   * We're also adding events without them having any meaning. Proper
   * event tracing including consistency checks will also be done later.
   *
   * @param nsweeps How many mesh sweeps to simulate.
   * @param nEventsToKeep How many events per particle the database should keep.
   *  If < `nsweeps`, then events from the database will be purged.
   */
  void testAddingParticleMovingEvents(
    int  nsweeps       = 100,
    int  nEventsToKeep = 1000,
    bool verbose       = false
  );

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
   * Test a particle walking through the box using lift-drop vertex
   * assignments.
   */
  void testParticleWalk();

  /**
   * Particles are identified in the events database using their unique ID.
   * However, if we have periodic boundary conditions, we may find that
   * particles with the same ID exist several times on the same tree, on
   * either side of the boundary, provided the tree spans both edges of the
   * domain. So check that we indeed find the correct ancestors.
   */
  void testPeriodicBoundaryConditions();

  /**
   * Test particle assignments to sieve sets.
   */
  void testSieveSet();

public:
  TestHelpers();
  virtual void run() override;
};
