// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiscaleTransitionsTest.h"

#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/particles/MultiscaleTransitions.h"
#include "peano4/particles/particles.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int           SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr double        MeshSize          = 1.0 / (SubdivisionFactor * SubdivisionFactor);
  constexpr int           LastVertex        = peano4::VerticesPerCell - 1;
  constexpr std::uint8_t  AllVertices       = (1U << peano4::VerticesPerCell) - 1U;
  constexpr std::uint32_t AllAdjacentCells  = (1U << peano4::CellsPerNeighbourhood) - 1U;
  constexpr double        FittingRadius     = 0.25 * MeshSize;
  constexpr double        OversizedRadius   = MeshSize;

  /**
   * Returns an unrefined cell at the origin corner of its parent, with every
   * vertex, every adjacent cell and the parent cell local.
   */
  peano4::grid::GridTraversalEvent leafCellEvent() {
    peano4::grid::GridTraversalEvent event;
    event.setX(tarch::la::Vector<DIMENSIONS, double>(0.5 * MeshSize));
    event.setH(tarch::la::Vector<DIMENSIONS, double>(MeshSize));
    event.setRelativePositionToFather(tarch::la::Vector<DIMENSIONS, int>(0));
    event.setIsVertexLocal(AllVertices);
    event.setIsParentVertexLocal(AllVertices);
    event.setIsAdjacentCellLocal(AllAdjacentCells);
    event.setIsCellLocal(true);
    event.setIsParentCellLocal(true);
    return event;
  }

  /**
   * Returns the point that lies offsetInMeshWidths cell widths away from the
   * cell's origin corner along every axis.
   */
  tarch::la::Vector<DIMENSIONS, double> pointInCell(const double offsetInMeshWidths) {
    return tarch::la::Vector<DIMENSIONS, double>(offsetInMeshWidths * MeshSize);
  }
} // namespace

peano4::particles::tests::MultiscaleTransitionsTest::MultiscaleTransitionsTest() :
  TestCase("peano4::particles::tests::MultiscaleTransitionsTest") {}

void peano4::particles::tests::MultiscaleTransitionsTest::run() {
  testMethod(testVertexKeepsAndDropsAParticleWithinItsReach);
  testMethod(testVertexLiftsAParticleBeyondItsReachToTheParentVertex);
  testMethod(testLiftSievesGloballyWithoutALocalParent);
  testMethod(testRemoteParticleIsKept);
  testMethod(testSieveWaitsForTheLevelTheParticleFitsInto);
  testMethod(testReassociationWithinCell);
  testMethod(testAdjacentCellsOwningParticle);
}

/**
 * A particle within half a cell width of its vertex stays with that vertex:
 * The vertex keeps it, drops it from the next coarser level and sieves it
 * from the global sieve set.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testVertexKeepsAndDropsAParticleWithinItsReach() {
  const peano4::dataview::VertexMarker originVertex(leafCellEvent(), 0);
  const auto                           particle = pointInCell(0.25);

  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, FittingRadius, particle, originVertex),
    peano4::particles::ParticleReassociationInstructionKeep,
    originVertex.toString()
  );
  validateWithParams1(peano4::particles::dropParticle(FittingRadius, particle, originVertex), originVertex.toString());
  validateWithParams1(peano4::particles::sieveParticle(FittingRadius, particle, originVertex), originVertex.toString());
  validateWithParams1(
    not peano4::particles::particleWillBeDroppedFurther(FittingRadius, originVertex),
    originVertex.toString()
  );
}

/**
 * A particle beyond half a cell width of its vertex leaves that vertex. The
 * lift names the parent vertex closest to the particle, and the vertex of the
 * same cell that is within reach keeps and drops the particle instead.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testVertexLiftsAParticleBeyondItsReachToTheParentVertex() {
  const auto                           event = leafCellEvent();
  const peano4::dataview::VertexMarker originVertex(event, 0);
  const peano4::dataview::VertexMarker oppositeVertex(event, LastVertex);
  const auto                           particle = pointInCell(0.75);

  // The cell sits at the origin corner of its parent, so the particle lies
  // below the parent's centre along every axis: Parent vertex 0 takes it.
  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, FittingRadius, particle, originVertex),
    0,
    originVertex.toString()
  );
  validateWithParams1(
    not peano4::particles::dropParticle(FittingRadius, particle, originVertex),
    originVertex.toString()
  );

  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, FittingRadius, particle, oppositeVertex),
    peano4::particles::ParticleReassociationInstructionKeep,
    oppositeVertex.toString()
  );
  validateWithParams1(
    peano4::particles::dropParticle(FittingRadius, particle, oppositeVertex),
    oppositeVertex.toString()
  );

  // A search radius the cell cannot hold lifts the particle even from the
  // vertex within reach.
  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, OversizedRadius, particle, oppositeVertex),
    0,
    oppositeVertex.toString()
  );
  validateWithParams1(
    not peano4::particles::dropParticle(OversizedRadius, particle, oppositeVertex),
    oppositeVertex.toString()
  );
}

/**
 * A lift that cannot hand the particle to a local parent vertex sends it to
 * the global sieve set: When the parent cell is remote, and when the particle
 * has left the parent cell.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testLiftSievesGloballyWithoutALocalParent() {
  auto event = leafCellEvent();
  event.setIsParentCellLocal(false);
  const peano4::dataview::VertexMarker vertexWithRemoteParent(event, 0);
  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, FittingRadius, pointInCell(0.75), vertexWithRemoteParent),
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    vertexWithRemoteParent.toString()
  );

  const peano4::dataview::VertexMarker originVertex(leafCellEvent(), 0);
  const auto                           particleOutsideParent = pointInCell(SubdivisionFactor + 0.25);
  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(true, FittingRadius, particleOutsideParent, originVertex),
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    originVertex.toString()
  );
}

/** A particle that is not local stays where it is, whatever its search radius. */
void peano4::particles::tests::MultiscaleTransitionsTest::testRemoteParticleIsKept() {
  const peano4::dataview::VertexMarker originVertex(leafCellEvent(), 0);
  validateEqualsWithParams1(
    peano4::particles::liftParticleAssociatedWithVertex(false, OversizedRadius, pointInCell(4.0), originVertex),
    peano4::particles::ParticleReassociationInstructionKeep,
    originVertex.toString()
  );
}

/**
 * A refined vertex sieves only particles that do not fit into the next finer
 * level; a particle that fits the children is dropped further instead.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testSieveWaitsForTheLevelTheParticleFitsInto() {
  auto event = leafCellEvent();
  event.setHasBeenRefined(AllVertices);
  const peano4::dataview::VertexMarker refinedVertex(event, 0);
  const auto                           refinedCell = event.getCellMarker();
  const auto                           particle    = pointInCell(0.25);

  const double radiusFittingTheChildren = 0.25 * MeshSize / SubdivisionFactor;
  validateWithParams1(
    not peano4::particles::sieveParticle(radiusFittingTheChildren, particle, refinedVertex),
    refinedVertex.toString()
  );
  validateWithParams1(
    peano4::particles::particleWillBeDroppedFurther(radiusFittingTheChildren, refinedVertex),
    refinedVertex.toString()
  );
  validateWithParams1(
    peano4::particles::particleWillBeDroppedFurther(radiusFittingTheChildren, refinedCell),
    refinedCell.toString()
  );

  const double radiusFittingThisLevelOnly = 0.4 * MeshSize;
  validateWithParams1(
    peano4::particles::sieveParticle(radiusFittingThisLevelOnly, particle, refinedVertex),
    refinedVertex.toString()
  );
  validateWithParams1(
    not peano4::particles::particleWillBeDroppedFurther(radiusFittingThisLevelOnly, refinedVertex),
    refinedVertex.toString()
  );
  validateWithParams1(
    not peano4::particles::particleWillBeDroppedFurther(radiusFittingThisLevelOnly, refinedCell),
    refinedCell.toString()
  );
}

/**
 * Within a cell, a particle moves to the vertex closest to it, stays where it
 * is when it already has that vertex, and is lifted to the parent vertex when
 * its search radius exceeds the cell.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testReassociationWithinCell() {
  auto       event    = leafCellEvent();
  const auto cell     = event.getCellMarker();
  const auto particle = pointInCell(0.75);

  validateEqualsWithParams1(
    peano4::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      true,
      FittingRadius,
      particle,
      cell,
      0
    ),
    LastVertex,
    cell.toString()
  );
  validateEqualsWithParams1(
    peano4::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      true,
      FittingRadius,
      particle,
      cell,
      LastVertex
    ),
    peano4::particles::ParticleReassociationInstructionKeep,
    cell.toString()
  );
  validateEqualsWithParams1(
    peano4::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      true,
      OversizedRadius,
      particle,
      cell,
      LastVertex
    ),
    peano4::VerticesPerCell,
    cell.toString()
  );
  validateEqualsWithParams1(
    peano4::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      false,
      OversizedRadius,
      particle,
      cell,
      0
    ),
    peano4::particles::ParticleReassociationInstructionKeep,
    cell.toString()
  );

  event.setIsParentCellLocal(false);
  const auto cellWithRemoteParent = event.getCellMarker();
  validateEqualsWithParams1(
    peano4::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      true,
      OversizedRadius,
      particle,
      cellWithRemoteParent,
      LastVertex
    ),
    peano4::particles::ParticleReassociationInstructionSieveGlobally,
    cellWithRemoteParent.toString()
  );

  validateNumericalEquals(
    peano4::particles::relativeSpatialOwnershipTolerance(cell),
    peano4::particles::ReleaseOwnershipSpatialSortingTolerance * MeshSize * tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
}

/**
 * Of the cells around a vertex, exactly the one that contains the particle
 * claims it; the cell index follows the direction of the particle from the
 * vertex along every axis.
 */
void peano4::particles::tests::MultiscaleTransitionsTest::testAdjacentCellsOwningParticle() {
  const peano4::dataview::VertexMarker originVertex(leafCellEvent(), 0);

  const auto owningCellsAbove = peano4::particles::getAdjacentCellsOwningParticle(pointInCell(0.25), originVertex);
  validateEqualsWithParams1(owningCellsAbove.count(), 1U, owningCellsAbove);
  validateWithParams1(owningCellsAbove[LastVertex], owningCellsAbove);

  const auto owningCellsBelow = peano4::particles::getAdjacentCellsOwningParticle(pointInCell(-0.25), originVertex);
  validateEqualsWithParams1(owningCellsBelow.count(), 1U, owningCellsBelow);
  validateWithParams1(owningCellsBelow[0], owningCellsBelow);
}
