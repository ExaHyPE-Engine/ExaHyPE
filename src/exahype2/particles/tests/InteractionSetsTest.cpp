// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InteractionSetsTest.h"

#include "exahype2/particles/InteractionSets.h"
#include "exahype2/particles/tests/TestParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/particles/particles.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace {
  using Particle    = exahype2::particles::tests::TestParticle;
  using ParticleSet = exahype2::particles::tests::TestParticleSet;
  using Vertices    = exahype2::particles::tests::TestVertices;

  constexpr int           SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr double        MeshSize          = 1.0 / (SubdivisionFactor * SubdivisionFactor);
  constexpr std::uint8_t  AllVertices       = (1U << peano4::VerticesPerCell) - 1U;
  constexpr std::uint32_t AllAdjacentCells  = (1U << peano4::CellsPerNeighbourhood) - 1U;
  constexpr int           LastVertex        = peano4::VerticesPerCell - 1;

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

  peano4::dataview::CellMarker leafCellMarker() { return leafCellEvent().getCellMarker(); }

  /** Returns the point offsetInMeshWidths cell widths away from the origin corner of the cell. */
  tarch::la::Vector<DIMENSIONS, double> pointInCell(const double offsetInMeshWidths) {
    return tarch::la::Vector<DIMENSIONS, double>(offsetInMeshWidths * MeshSize);
  }

  /**
   * Creates a particle at position and flags the cells around vertex that contain it, so the
   * flags agree with the geometry the cell marker describes.
   */
  Particle* createParticleAtVertex(const tarch::la::Vector<DIMENSIONS, double>& position, const int vertex) {
    auto* const particle = new Particle();
    particle->setX(position);
    particle->setSearchRadius(0.25 * MeshSize);
    const peano4::dataview::VertexMarker marker(leafCellEvent(), vertex);
    peano4::particles::updateContainedInAdjacentCellProperty(marker, *particle);
    return particle;
  }
} // namespace

exahype2::particles::tests::InteractionSetsTest::InteractionSetsTest() :
  tarch::tests::TestCase("exahype2::particles::tests::InteractionSetsTest") {}

void exahype2::particles::tests::InteractionSetsTest::run() {
  testMethod(testCollectParticlesOfCell);
  testMethod(testUpdateContainedInAdjacentCellOfLocalParticles);
  testMethod(testReleaseParticlesOfCell);
}

/**
 * The cell appends the particles of its vertices to both lists in vertex order and flags the
 * local particles it contains.
 */
void exahype2::particles::tests::InteractionSetsTest::testCollectParticlesOfCell() {
  const auto cell = leafCellMarker();

  ParticleSet    activeVertices[peano4::VerticesPerCell];
  ParticleSet    localVertices[peano4::VerticesPerCell];
  const Vertices activeGridVertices(activeVertices);
  const Vertices localGridVertices(localVertices);

  auto* const firstActiveParticle  = createParticleAtVertex(pointInCell(0.25), 1);
  auto* const secondActiveParticle = createParticleAtVertex(pointInCell(0.25), 2);
  activeVertices[1].addParticle(firstActiveParticle);
  activeVertices[2].addParticle(secondActiveParticle);

  auto* const containedParticle      = createParticleAtVertex(pointInCell(0.25), 0);
  auto* const outsideParticle        = createParticleAtVertex(pointInCell(-0.25), 0);
  auto* const oppositeCornerParticle = createParticleAtVertex(pointInCell(0.75), LastVertex);
  localVertices[0].addParticle(containedParticle);
  localVertices[0].addParticle(outsideParticle);
  localVertices[LastVertex].addParticle(oppositeCornerParticle);

  std::list<Particle*> activeParticles;
  std::list<Particle*> localParticles;
  std::vector<bool>    localParticleIsContainedInCell;

  const int numberOfActiveParticles = exahype2::particles::collectParticlesOfCell(
    cell,
    activeGridVertices,
    localGridVertices,
    activeParticles,
    localParticles,
    localParticleIsContainedInCell
  );

  validateEquals(numberOfActiveParticles, 2);
  validateEquals(static_cast<int>(activeParticles.size()), 2);
  validateEquals(activeParticles.front(), firstActiveParticle);
  validateEquals(activeParticles.back(), secondActiveParticle);

  validateEquals(static_cast<int>(localParticles.size()), 3);
  validateEquals(localParticles.front(), containedParticle);
  validateEquals(localParticles.back(), oppositeCornerParticle);

  validateEquals(static_cast<int>(localParticleIsContainedInCell.size()), 3);
  validateTrue(localParticleIsContainedInCell[0]);
  validateFalse(localParticleIsContainedInCell[1]);
  validateTrue(localParticleIsContainedInCell[2]);

  for (auto& vertex : activeVertices) {
    vertex.deleteParticles();
  }
  for (auto& vertex : localVertices) {
    vertex.deleteParticles();
  }
}

/** The update flags exactly the one cell around the vertex of a particle that contains it. */
void exahype2::particles::tests::InteractionSetsTest::testUpdateContainedInAdjacentCellOfLocalParticles() {
  const auto cell = leafCellMarker();

  ParticleSet    localVertices[peano4::VerticesPerCell];
  const Vertices localGridVertices(localVertices);

  auto* const particle = createParticleAtVertex(pointInCell(0.25), 0);
  particle->setContainedInAdjacentCell(std::bitset<peano4::CellsPerVertex>());
  localVertices[0].addParticle(particle);

  exahype2::particles::updateContainedInAdjacentCellOfLocalParticles(cell, localGridVertices);

  validateEquals(static_cast<int>(particle->getContainedInAdjacentCell().count()), 1);
  validateTrue(particle->getContainedInAdjacentCell()[peano4::CellsPerVertex - 1]);

  for (auto& vertex : localVertices) {
    vertex.deleteParticles();
  }
}

/**
 * The release leaves the active list with the particles it carried before the cell added its own,
 * although a vertex of the cell gains a particle in between.
 */
void exahype2::particles::tests::InteractionSetsTest::testReleaseParticlesOfCell() {
  const auto cell = leafCellMarker();

  ParticleSet    activeVertices[peano4::VerticesPerCell];
  ParticleSet    localVertices[peano4::VerticesPerCell];
  const Vertices activeGridVertices(activeVertices);
  const Vertices localGridVertices(localVertices);

  activeVertices[1].addParticle(createParticleAtVertex(pointInCell(0.25), 1));
  activeVertices[2].addParticle(createParticleAtVertex(pointInCell(0.25), 2));

  Particle             particleOfACoarserCell;
  std::list<Particle*> activeParticles{&particleOfACoarserCell};
  std::list<Particle*> localParticles;
  std::vector<bool>    localParticleIsContainedInCell;

  const int numberOfActiveParticles = exahype2::particles::collectParticlesOfCell(
    cell,
    activeGridVertices,
    localGridVertices,
    activeParticles,
    localParticles,
    localParticleIsContainedInCell
  );
  validateEquals(static_cast<int>(activeParticles.size()), 3);

  activeVertices[3].addParticle(createParticleAtVertex(pointInCell(0.25), 3));
  exahype2::particles::releaseParticlesOfCell(numberOfActiveParticles, activeParticles);

  validateEquals(static_cast<int>(activeParticles.size()), 1);
  validateEquals(activeParticles.front(), &particleOfACoarserCell);

  for (auto& vertex : activeVertices) {
    vertex.deleteParticles();
  }
}
