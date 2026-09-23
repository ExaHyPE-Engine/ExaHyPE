// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InsertParticlesByCoordinatesTest.h"

#include "exahype2/particles/InsertParticlesByCoordinates.h"
#include "exahype2/particles/tests/TestParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace {
  using Particle    = exahype2::particles::tests::TestParticle;
  using ParticleSet = exahype2::particles::tests::TestParticleSet;

  constexpr int    SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr double MeshSize          = 1.0 / (SubdivisionFactor * SubdivisionFactor);
  constexpr int    TreeId            = 6;

  /** Returns an unrefined local cell whose origin corner sits at the origin. */
  peano4::dataview::CellMarker leafCellMarker() {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(MeshSize);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(0.5 * MeshSize);
    marker.flags  = peano4::dataview::CellMarker::IsLocal | peano4::dataview::CellMarker::IsParentLocal;
    return marker;
  }

  /** Returns the point offsetInMeshWidths cell widths away from the origin corner of the cell. */
  tarch::la::Vector<DIMENSIONS, double> pointInCell(const double offsetInMeshWidths) {
    return tarch::la::Vector<DIMENSIONS, double>(offsetInMeshWidths * MeshSize);
  }
} // namespace

exahype2::particles::tests::InsertParticlesByCoordinatesTest::InsertParticlesByCoordinatesTest() :
  tarch::tests::TestCase("exahype2::particles::tests::InsertParticlesByCoordinatesTest") {}

void exahype2::particles::tests::InsertParticlesByCoordinatesTest::run() {
  testMethod(testInsertsOneParticlePerOwnedCoordinate);
  testMethod(testRefinedCellInsertsNoParticle);
}

/**
 * Only the coordinate the cell owns yields a particle, and that particle reaches the vertex of
 * the cell closest to it as a local particle.
 */
void exahype2::particles::tests::InsertParticlesByCoordinatesTest::testInsertsOneParticlePerOwnedCoordinate() {
  const auto cell = leafCellMarker();

  ParticleSet vertexPool[peano4::VerticesPerCell + 1];
  int         slots[peano4::VerticesPerCell];
  for (int vertex = 0; vertex < peano4::VerticesPerCell; vertex++) {
    slots[vertex] = vertex;
  }
  peano4::dataview::VertexEnumerator<ParticleSet> vertices(vertexPool, slots, peano4::VerticesPerCell);

  const std::array<tarch::la::Vector<DIMENSIONS, double>, 3> coordinates{
    pointInCell(0.25),
    pointInCell(4.0),
    pointInCell(0.0)
  };
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(-1.0);

  int numberOfInitialisedParticles = 0;
  exahype2::particles::insertParticlesByCoordinates<Particle>(
    cell,
    coordinates,
    domainOffset,
    vertices,
    TreeId,
    [&]([[maybe_unused]] auto* const particle) {
      particle->setNumber(0, TreeId);
      particle->setNumber(1, numberOfInitialisedParticles);
      numberOfInitialisedParticles++;
    }
  );

  validateEquals(numberOfInitialisedParticles, 1);
  validateEquals(vertexPool[0].size(), 1);

  auto* const particle = *vertexPool[0].begin();
  validateTrue(particle->getParallelState() == Particle::ParallelState::Local);
  validateEquals(particle->getX(), pointInCell(0.25));
  validateEquals(particle->getCellH(), cell.getH());
  validateEquals(particle->getNumber(0), TreeId);
  validateEquals(particle->getNumber(1), 0);
  validateNumericalEquals(particle->getSearchRadius(), 0.0);

  for (auto& vertex : vertexPool) {
    vertex.deleteParticles();
  }
}

/** A cell that gains children in this sweep inserts no particle. */
void exahype2::particles::tests::InsertParticlesByCoordinatesTest::testRefinedCellInsertsNoParticle() {
  auto cell  = leafCellMarker();
  cell.flags = static_cast<std::uint16_t>(cell.flags | peano4::dataview::CellMarker::WillBeRefined);

  ParticleSet vertexPool[peano4::VerticesPerCell + 1];
  int         slots[peano4::VerticesPerCell];
  for (int vertex = 0; vertex < peano4::VerticesPerCell; vertex++) {
    slots[vertex] = vertex;
  }
  peano4::dataview::VertexEnumerator<ParticleSet> vertices(vertexPool, slots, peano4::VerticesPerCell);

  const std::array<tarch::la::Vector<DIMENSIONS, double>, 1> coordinates{pointInCell(0.25)};

  int numberOfInitialisedParticles = 0;
  exahype2::particles::insertParticlesByCoordinates<Particle>(
    cell,
    coordinates,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    vertices,
    TreeId,
    [&]([[maybe_unused]] auto* const particle) { numberOfInitialisedParticles++; }
  );

  validateEquals(numberOfInitialisedParticles, 0);
  validateEquals(vertexPool[0].size(), 0);
}
