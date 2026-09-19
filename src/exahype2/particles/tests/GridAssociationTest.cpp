// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridAssociationTest.h"

#include "exahype2/particles/GridAssociation.h"
#include "exahype2/particles/tests/TestParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
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
  constexpr double        FittingRadius     = 0.25 * MeshSize;
  constexpr double        OversizedRadius   = MeshSize;
  constexpr int           TreeId            = 4;

  /**
   * Returns an unrefined cell at the origin corner of its parent, with every vertex, every
   * adjacent cell and the parent cell local.
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

  /** Returns the point offsetInMeshWidths cell widths away from the origin corner of the cell. */
  tarch::la::Vector<DIMENSIONS, double> pointInCell(const double offsetInMeshWidths) {
    return tarch::la::Vector<DIMENSIONS, double>(offsetInMeshWidths * MeshSize);
  }

  Particle* createParticle(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 searchRadius,
    const int                                    particleId
  ) {
    auto* const particle = new Particle();
    particle->setX(position);
    particle->setSearchRadius(searchRadius);
    particle->setPartid(particleId);
    particle->setNumber(0, TreeId);
    particle->setNumber(1, particleId);
    return particle;
  }

  tarch::la::Vector<DIMENSIONS, double> parentCellSize(const peano4::dataview::VertexMarker& marker) {
    return static_cast<double>(SubdivisionFactor) * marker.getH();
  }
} // namespace

exahype2::particles::tests::GridAssociationTest::GridAssociationTest() :
  tarch::tests::TestCase("exahype2::particles::tests::GridAssociationTest") {}

void exahype2::particles::tests::GridAssociationTest::run() {
  testMethod(testStatisticsClearAndRecord);
  testMethod(testDropParticlesOntoTheFineVertex);
  testMethod(testDropParticlesIntoTheSieveSetOfASubtree);
  testMethod(testSieveParticlesIntoVertex);
  testMethod(testLiftOrReassignKeepsAParticleWithinReach);
  testMethod(testLiftOrReassignWrapsAParticleAtAPeriodicWall);
  testMethod(testLiftOrReassignLiftsAParticleToTheParentVertex);
  testMethod(testLiftOrReassignSievesAParticleGlobally);
  testMethod(testLiftAllParticlesToTheParentVertex);
  testMethod(testLiftAllParticlesIntoTheSieveSet);
}

/** The recorded statistics carry the six counters in the order the particle container expects. */
void exahype2::particles::tests::GridAssociationTest::testStatisticsClearAndRecord() {
  ParticleSet::clearTestState();

  exahype2::particles::LiftDropStatistics statistics;
  statistics.numberOfLifts                                = 1;
  statistics.numberOfDrops                                = 2;
  statistics.numberOfLiftsIntoSieveSet                    = 3;
  statistics.numberOfDropsFromSieveSet                    = 4;
  statistics.numberOfDropsIntoHorizontalTreeDecomposition = 5;
  statistics.record<ParticleSet>();

  const auto& recorded = ParticleSet::getRecordedLiftDropStatistics();
  validateEquals(recorded[0], 1);
  validateEquals(recorded[1], 2);
  validateEquals(recorded[2], 3);
  validateEquals(recorded[3], 4);
  validateEquals(recorded[4], 5);

  statistics.clear();
  validateEquals(statistics.numberOfLifts, 0);
  validateEquals(statistics.numberOfDrops, 0);
  validateEquals(statistics.numberOfLiftsIntoSieveSet, 0);
  validateEquals(statistics.numberOfDropsFromSieveSet, 0);
  validateEquals(statistics.numberOfDropsIntoHorizontalTreeDecomposition, 0);

  ParticleSet::clearTestState();
}

/** A drop moves the particle that fits the level of the vertex and leaves the other one. */
void exahype2::particles::tests::GridAssociationTest::testDropParticlesOntoTheFineVertex() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);
  const auto                           parentVertexPosition = peano4::dataview::reconstructXOfParentVertex(marker, 0);

  ParticleSet    fineGridVertexParticles;
  ParticleSet    parentVertices[peano4::VerticesPerCell];
  const Vertices coarseGridVertexParticles(parentVertices);

  auto* const droppedParticle = createParticle(pointInCell(0.25), FittingRadius, 101);
  auto* const keptParticle    = createParticle(pointInCell(0.25), OversizedRadius, 102);
  parentVertices[0].addParticle(droppedParticle);
  parentVertices[0].addParticle(keptParticle);
  exahype2::particles::tests::recordInitialParticleAssignment(
    *droppedParticle,
    parentVertexPosition,
    parentCellSize(marker),
    TreeId
  );

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::dropParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfDrops, 1);
  validateEquals(statistics.numberOfDropsIntoHorizontalTreeDecomposition, 0);
  validateEquals(fineGridVertexParticles.size(), 1);
  validateEquals(parentVertices[0].size(), 1);
  validateEquals(*fineGridVertexParticles.begin(), droppedParticle);
  validateEquals(*parentVertices[0].begin(), keptParticle);
  validateEquals(droppedParticle->getCellH(), marker.getH());

  fineGridVertexParticles.deleteParticles();
  for (auto& vertex : parentVertices) {
    vertex.deleteParticles();
  }
  ParticleSet::clearTestState();
}

/**
 * A particle that a finer level takes as well leaves the parent vertex for the sieve set, and
 * the vertex of the marker stays empty.
 */
void exahype2::particles::tests::GridAssociationTest::testDropParticlesIntoTheSieveSetOfASubtree() {
  ParticleSet::clearTestState();

  auto event = leafCellEvent();
  event.setIsVertexParentOfSubtree(AllVertices);
  event.setHasBeenRefined(AllVertices);
  const peano4::dataview::VertexMarker marker(event, 0);

  ParticleSet    fineGridVertexParticles;
  ParticleSet    parentVertices[peano4::VerticesPerCell];
  const Vertices coarseGridVertexParticles(parentVertices);
  parentVertices[0].addParticle(createParticle(pointInCell(0.25), MeshSize / 12.0, 111));

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::dropParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfDropsIntoHorizontalTreeDecomposition, 1);
  validateEquals(statistics.numberOfDrops, 0);
  validateEquals(fineGridVertexParticles.size(), 0);
  validateEquals(parentVertices[0].size(), 0);
  validateEquals(static_cast<int>(ParticleSet::getParticlesLiftedIntoSieveSet().size()), 1);

  ParticleSet::clearTestState();
}

/** The sieve hands every pending particle to the vertex and counts one drop per particle. */
void exahype2::particles::tests::GridAssociationTest::testSieveParticlesIntoVertex() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker    marker(leafCellEvent(), 0);
  ParticleSet                             fineGridVertexParticles;
  exahype2::particles::LiftDropStatistics statistics;

  exahype2::particles::sieveParticlesIntoVertex(marker, fineGridVertexParticles, TreeId, statistics);
  validateEquals(statistics.numberOfDropsFromSieveSet, 0);
  validateEquals(fineGridVertexParticles.size(), 0);

  ParticleSet::addParticleToBeSieved(createParticle(pointInCell(0.25), FittingRadius, 121));
  ParticleSet::addParticleToBeSieved(createParticle(pointInCell(0.25), FittingRadius, 122));
  validateTrue(ParticleSet::hasParticlesToBeSievedIntoVertices());

  exahype2::particles::sieveParticlesIntoVertex(marker, fineGridVertexParticles, TreeId, statistics);
  validateEquals(statistics.numberOfDropsFromSieveSet, 2);
  validateEquals(fineGridVertexParticles.size(), 2);
  validateFalse(ParticleSet::hasParticlesToBeSievedIntoVertices());

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}

/** A particle within half a cell width of its vertex keeps that vertex and updates its owner cell. */
void exahype2::particles::tests::GridAssociationTest::testLiftOrReassignKeepsAParticleWithinReach() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);
  ParticleSet                          fineGridVertexParticles;
  ParticleSet                          parentVertices[peano4::VerticesPerCell];
  const Vertices                       coarseGridVertexParticles(parentVertices);

  auto* const particle = createParticle(pointInCell(0.25), FittingRadius, 131);
  fineGridVertexParticles.addParticle(particle);

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftOrReassignParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfLifts, 0);
  validateEquals(statistics.numberOfLiftsIntoSieveSet, 0);
  validateEquals(fineGridVertexParticles.size(), 1);
  validateEquals(static_cast<int>(particle->getContainedInAdjacentCell().count()), 1);
  validateTrue(particle->getContainedInAdjacentCell()[peano4::CellsPerVertex - 1]);

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}

/** A particle beyond a periodic wall returns to the opposite wall before the lift reads it. */
void exahype2::particles::tests::GridAssociationTest::testLiftOrReassignWrapsAParticleAtAPeriodicWall() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);
  ParticleSet                          fineGridVertexParticles;
  ParticleSet                          parentVertices[peano4::VerticesPerCell];
  const Vertices                       coarseGridVertexParticles(parentVertices);

  auto position = pointInCell(0.25);
  position(0) += 1.0;
  auto* const particle = createParticle(position, FittingRadius, 141);
  fineGridVertexParticles.addParticle(particle);

  std::bitset<DIMENSIONS> firstAxisPeriodic;
  firstAxisPeriodic[0] = true;

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftOrReassignParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    firstAxisPeriodic,
    "TestParticle",
    TreeId,
    statistics
  );

  validateNumericalEquals(particle->getX()(0), 0.25 * MeshSize);
  validateEquals(fineGridVertexParticles.size(), 1);
  validateEquals(statistics.numberOfLifts, 0);

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}

/** A particle beyond the reach of its vertex moves to the parent vertex closest to it. */
void exahype2::particles::tests::GridAssociationTest::testLiftOrReassignLiftsAParticleToTheParentVertex() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);
  ParticleSet                          fineGridVertexParticles;
  ParticleSet                          parentVertices[peano4::VerticesPerCell];
  const Vertices                       coarseGridVertexParticles(parentVertices);

  auto* const particle = createParticle(pointInCell(0.75), FittingRadius, 151);
  fineGridVertexParticles.addParticle(particle);
  exahype2::particles::tests::recordInitialParticleAssignment(*particle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftOrReassignParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfLifts, 1);
  validateEquals(fineGridVertexParticles.size(), 0);
  validateEquals(parentVertices[0].size(), 1);
  validateEquals(*parentVertices[0].begin(), particle);
  validateEquals(particle->getCellH(), parentCellSize(marker));

  for (auto& vertex : parentVertices) {
    vertex.deleteParticles();
  }
  ParticleSet::clearTestState();
}

/** A particle whose parent cell is remote leaves the tree through the sieve set. */
void exahype2::particles::tests::GridAssociationTest::testLiftOrReassignSievesAParticleGlobally() {
  ParticleSet::clearTestState();

  auto event = leafCellEvent();
  event.setIsParentCellLocal(false);
  const peano4::dataview::VertexMarker marker(event, 0);

  ParticleSet    fineGridVertexParticles;
  ParticleSet    parentVertices[peano4::VerticesPerCell];
  const Vertices coarseGridVertexParticles(parentVertices);

  auto* const particle = createParticle(pointInCell(0.75), FittingRadius, 161);
  fineGridVertexParticles.addParticle(particle);
  exahype2::particles::tests::recordInitialParticleAssignment(*particle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftOrReassignParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfLiftsIntoSieveSet, 1);
  validateEquals(statistics.numberOfLifts, 0);
  validateEquals(fineGridVertexParticles.size(), 0);
  validateEquals(static_cast<int>(ParticleSet::getParticlesLiftedIntoSieveSet().size()), 1);

  ParticleSet::clearTestState();
}

/** The destroyed vertex hands every particle to the parent vertex nearest to it. */
void exahype2::particles::tests::GridAssociationTest::testLiftAllParticlesToTheParentVertex() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);
  ParticleSet                          fineGridVertexParticles;
  ParticleSet                          parentVertices[peano4::VerticesPerCell];
  const Vertices                       coarseGridVertexParticles(parentVertices);

  auto* const firstParticle  = createParticle(pointInCell(0.25), FittingRadius, 171);
  auto* const secondParticle = createParticle(pointInCell(0.35), FittingRadius, 172);
  fineGridVertexParticles.addParticle(firstParticle);
  fineGridVertexParticles.addParticle(secondParticle);
  exahype2::particles::tests::recordInitialParticleAssignment(*firstParticle, marker.getX(), marker.getH(), TreeId);
  exahype2::particles::tests::recordInitialParticleAssignment(*secondParticle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftAllParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfLifts, 2);
  validateEquals(statistics.numberOfLiftsIntoSieveSet, 0);
  validateEquals(fineGridVertexParticles.size(), 0);
  validateEquals(parentVertices[0].size(), 2);
  validateEquals(firstParticle->getCellH(), parentCellSize(marker));
  validateEquals(secondParticle->getCellH(), parentCellSize(marker));

  for (auto& vertex : parentVertices) {
    vertex.deleteParticles();
  }
  ParticleSet::clearTestState();
}

/** A remote parent vertex takes no particle, so the destroyed vertex fills the sieve set. */
void exahype2::particles::tests::GridAssociationTest::testLiftAllParticlesIntoTheSieveSet() {
  ParticleSet::clearTestState();

  auto event = leafCellEvent();
  event.setIsParentVertexLocal(static_cast<std::uint8_t>(0));
  const peano4::dataview::VertexMarker marker(event, 0);

  ParticleSet    fineGridVertexParticles;
  ParticleSet    parentVertices[peano4::VerticesPerCell];
  const Vertices coarseGridVertexParticles(parentVertices);

  auto* const particle = createParticle(pointInCell(0.25), FittingRadius, 181);
  fineGridVertexParticles.addParticle(particle);
  exahype2::particles::tests::recordInitialParticleAssignment(*particle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::LiftDropStatistics statistics;
  exahype2::particles::liftAllParticles(
    marker,
    fineGridVertexParticles,
    coarseGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateEquals(statistics.numberOfLiftsIntoSieveSet, 1);
  validateEquals(statistics.numberOfLifts, 0);
  validateEquals(fineGridVertexParticles.size(), 0);
  validateEquals(static_cast<int>(ParticleSet::getParticlesLiftedIntoSieveSet().size()), 1);

  ParticleSet::clearTestState();
}
