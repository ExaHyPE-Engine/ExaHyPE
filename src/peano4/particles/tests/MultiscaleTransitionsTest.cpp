// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiscaleTransitionsTest.h"

#include "peano4/particles/MultiscaleTransitions.h"
#include "peano4/particles/tests/TestParticle.h"

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

peano4::particles::tests::MultiscaleTransitionsTest::MultiscaleTransitionsTest() :
  TestCase("peano4::particles::tests::MultiscaleTransitionsTest") {}

void peano4::particles::tests::MultiscaleTransitionsTest::testSievePredicate() {
#if DIMENSIONS == 2
  const peano4::particles::tests::TestParticle particle({0.721487, 0.624479}, 0.0001);
  peano4::datamanagement::VertexMarker          marker;

  marker._cellCentre = {0.685185, 0.648148};
  marker._h          = {0.037037, 0.037037};
  marker._isHanging  = 0;
  marker._select     = 1;
  marker._isAdjacentCellLocal.set();

  const tarch::la::Vector<DIMENSIONS, double> cornerVertex1 = {
    marker._cellCentre(0) + 0.5 * marker._h(0),
    marker._cellCentre(1) - 0.5 * marker._h(1)
  };

  validateWithParams6(
    peano4::particles::sieveParticle(particle, marker),
    marker.toString(),
    particle._x,
    cornerVertex1,
    marker._h / 2.0,
    tarch::la::abs(marker._cellCentre - particle._x),
    tarch::la::abs(cornerVertex1 - particle._x)
  );
#endif
}

void peano4::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex01() {
#if DIMENSIONS == 2
  peano4::datamanagement::VertexMarker marker;

  const double meshSize = 0.111111;

  marker._cellCentre                      = {0.611111, 0.166667};
  marker._h                               = {meshSize, meshSize};
  marker._isHanging                       = 0;
  marker._hasBeenRefined                  = 1 + 2 + 4 + 8;
  marker._willBeRefined                   = 1 + 2 + 4 + 8;
  marker._isParentVertexLocal             = 1 + 2 + 4 + 8;
  marker._isParentCellLocal               = true;
  marker.relativePositionWithinFatherCell = {0, 0};
  marker._isAdjacentCellLocal.set();

  const tarch::la::Vector<DIMENSIONS, double>          x            = {0.554725, 0.166678};
  const double                                         searchRadius = 0.0185185;
  peano4::particles::ParticleReassociationInstruction liftInstruction;
  bool                                                 drop;

  // The bottom left vertex owns the particle.
  marker._select  = 0;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(
    true, // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = peano4::particles::dropParticle(searchRadius, x, marker);

  validateWithParams6(not drop, marker.toString(), x, liftInstruction, drop, (x - marker.getX()), searchRadius);

  // The top left vertex owns the particle.
  marker._select  = 2;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(
    true, // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = peano4::particles::dropParticle(searchRadius, x, marker);

  validateWithParams6(
    liftInstruction == peano4::particles::ParticleReassociationInstructionKeep,
    marker.toString(),
    x,
    liftInstruction,
    drop,
    (x - marker.getX()),
    searchRadius
  );
  validateWithParams6(drop, marker.toString(), x, liftInstruction, drop, (x - marker.getX()), searchRadius);
#endif
}

void peano4::particles::tests::MultiscaleTransitionsTest::run() {
  testMethod(testSievePredicate);
  testMethod(testLiftDropOfParticleAssociatedWithVertex01);
  testMethod(testLiftDropOfParticleAssociatedWithVertex02);
  testMethod(testLiftDropOfParticleAssociatedWithVertex03);
  testMethod(testLiftDropOfParticleAssociatedWithVertex04);
}

void peano4::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex02() {
#if DIMENSIONS == 2
  peano4::datamanagement::VertexMarker marker;

  const double meshSize = 0.037037;

  marker._cellCentre                      = {0.537037, 0.12963};
  marker._h                               = {meshSize, meshSize};
  marker._isHanging                       = 0;
  marker._hasBeenRefined                  = 0;
  marker._willBeRefined                   = 0;
  marker._isParentVertexLocal             = 1 + 2 + 4 + 8;
  marker._isParentCellLocal               = true;
  marker.relativePositionWithinFatherCell = {2, 0};
  marker._select                          = 2;
  marker._isAdjacentCellLocal.set();

  const tarch::la::Vector<DIMENSIONS, double>          x            = {0.504975, 0.1667};
  const double                                         searchRadius = 0.0185185;
  peano4::particles::ParticleReassociationInstruction liftInstruction;
  bool                                                 drop;

  // The top left vertex owns the particle, and the particle sits further left
  // and further up than that vertex, so the routine lifts it.
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(
    true, // ParallelState=Local
    searchRadius,
    x,
    marker
  );
  drop = peano4::particles::dropParticle(searchRadius, x, marker);

  validateWithParams7(
    liftInstruction == 3,
    marker.toString(),
    x,
    marker.getX(),
    liftInstruction,
    drop,
    (x - marker.getX()),
    searchRadius
  );
  validateWithParams6(not drop, marker.toString(), x, liftInstruction, drop, (x - marker.getX()), searchRadius);
#endif
}

void peano4::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex03() {
#if DIMENSIONS == 2
  peano4::datamanagement::VertexMarker marker;

  const double meshSize = 1.0 / 3.0 / 3.0;

  marker._cellCentre                      = {0.5 * meshSize, 3.5 * meshSize};
  marker._h                               = {meshSize, meshSize};
  marker._isLocal                         = 0b1111;
  marker._isHanging                       = 0b0000;
  marker._hasBeenRefined                  = 0b0000;
  marker._willBeRefined                   = 0b0000;
  marker._isParentVertexLocal             = 0b1111;
  marker._isParentCellLocal               = true;
  marker.relativePositionWithinFatherCell = {0, 0};
  marker._isAdjacentCellLocal             = 0b110110000;

  double                                               searchRadius = 0.00555556;
  const tarch::la::Vector<DIMENSIONS, double>          x            = {0.0555708, 0.322228};
  peano4::particles::ParticleReassociationInstruction liftInstruction;

  // Vertex 0 lifts the particle globally.
  marker._select  = 0;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(
    liftInstruction,
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    marker.toString()
  );

  // Vertex 1 keeps the particle.
  marker._select  = 1;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams5(
    liftInstruction,
    peano4::particles::ParticleReassociationInstructionKeep,
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Vertex 0 does not drop the particle.
  marker._select = 0;
  validateWithParams5(
    not peano4::particles::dropParticle(searchRadius, x, marker),
    peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Vertex 1 drops the particle.
  marker._select = 1;
  validateWithParams5(
    peano4::particles::dropParticle(searchRadius, x, marker),
    peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    peano4::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Repeat the checks from the boundary cell below on the other rank.
  marker._cellCentre(1) -= meshSize;
  marker._isAdjacentCellLocal.flip();

  // Vertex 2 does not drop the particle.
  marker._select = 2;
  validateWithParams1(not peano4::particles::dropParticle(searchRadius, x, marker), marker.toString());

  // Vertex 3 drops the particle.
  marker._select = 3;
  validateWithParams1(peano4::particles::dropParticle(searchRadius, x, marker), marker.toString());

  // Move to the cell to the left and grow the particle beyond the mesh size.
  marker._cellCentre(0) -= meshSize;
  searchRadius = 0.0555556;

  // Vertex 0 lifts the particle globally.
  marker._select  = 0;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(
    liftInstruction,
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    marker.toString()
  );

  // Vertex 1 lifts the particle globally too.
  marker._select  = 1;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams5(
    liftInstruction,
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Move on to the next coarser mesh level.
  marker._cellCentre = {1.5 * meshSize, 1.5 * meshSize};
  marker._select     = 2;
  marker._h          = {3.0 * meshSize, 3.0 * meshSize};

  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

#endif
}

void peano4::particles::tests::MultiscaleTransitionsTest::testLiftDropOfParticleAssociatedWithVertex04() {
#if DIMENSIONS == 2
  peano4::datamanagement::VertexMarker marker;

  const double meshSize = 1.0 / 3.0;

  marker._cellCentre                      = {0.166667, 0.5};
  marker._h                               = {meshSize, meshSize};
  marker._isHanging                       = 0;
  marker._hasBeenRefined                  = 0;
  marker._willBeRefined                   = 0;
  marker._isParentVertexLocal             = 0b1011;
  marker._isParentCellLocal               = true;
  marker.relativePositionWithinFatherCell = {2, 0};
  marker._select                          = 2;
  marker._isAdjacentCellLocal             = 0b000100110;

  const tarch::la::Vector<DIMENSIONS, double>          x = {4.99999996463071805231e-01, 5.00000003548028204570e-01};
  const double                                         searchRadius = 0.15;
  peano4::particles::ParticleReassociationInstruction liftInstruction;

  // Vertex 1 lifts the particle globally.
  marker._select  = 1;
  liftInstruction = peano4::particles::liftParticleAssociatedWithVertex(true, searchRadius, x, marker);
  validateEqualsWithParams1(
    liftInstruction,
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    marker.toString()
  );

  validateWithParams5(
    not peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select = 3;
  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Move on to the cell to the right.
  marker._cellCentre(0) += meshSize;
  marker._isAdjacentCellLocal = 0b000110111;

  marker._select = 0;
  validateWithParams5(
    not peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select = 2;
  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Move on to the diagonal top right cell.
  marker._cellCentre(1) += meshSize;
  marker._isAdjacentCellLocal = 0b000000110;

  marker._select = 0;
  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Repeat the checks on the other rank, starting with the left cell.
  marker._cellCentre          = {0.166667, 0.5};
  marker._isAdjacentCellLocal = 0b000100110;
  marker._isAdjacentCellLocal.flip();

  marker._select = 1;
  validateWithParams5(
    not peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  marker._select = 3;
  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );

  // Move on to the diagonal top right cell.
  marker._cellCentre(0) += meshSize;
  marker._cellCentre(1) += meshSize;
  marker._isAdjacentCellLocal = 0b000000110;
  marker._isAdjacentCellLocal.flip();

  marker._select = 0;
  validateWithParams5(
    peano4::particles::sieveParticle(searchRadius, x, marker),
    peano4::particles::internal::fitsIntoLevel(searchRadius, marker),
    marker.isContainedInAdjacentCells(
      x,
      0.5,
      peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
        * tarch::la::NUMERICAL_ZERO_DIFFERENCE
    ),
    2.0 * searchRadius,
    peano4::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker)
      * tarch::la::NUMERICAL_ZERO_DIFFERENCE,
    marker.toString()
  );
#endif
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
