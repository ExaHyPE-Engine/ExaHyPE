// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelStateTest.h"

#include "exahype2/particles/ParallelState.h"
#include "exahype2/particles/tests/TestParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace {
  using Particle    = exahype2::particles::tests::TestParticle;
  using ParticleSet = exahype2::particles::tests::TestParticleSet;

  constexpr int           SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr double        MeshSize          = 1.0 / (SubdivisionFactor * SubdivisionFactor);
  constexpr std::uint8_t  AllVertices       = (1U << peano4::VerticesPerCell) - 1U;
  constexpr std::uint32_t AllAdjacentCells  = (1U << peano4::CellsPerNeighbourhood) - 1U;
  constexpr double        FittingRadius     = 0.25 * MeshSize;
  constexpr int           TreeId            = 5;

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

  Particle* createParticle(const Particle::ParallelState state, const int particleId) {
    auto* const particle = new Particle();
    particle->setX(tarch::la::Vector<DIMENSIONS, double>(0.25 * MeshSize));
    particle->setSearchRadius(FittingRadius);
    particle->setPartid(particleId);
    particle->setNumber(0, TreeId);
    particle->setNumber(1, particleId);
    particle->setParallelState(state);
    return particle;
  }
} // namespace

exahype2::particles::tests::ParallelStateTest::ParallelStateTest() :
  tarch::tests::TestCase("exahype2::particles::tests::ParallelStateTest") {}

void exahype2::particles::tests::ParallelStateTest::run() {
  testMethod(testStatisticsClearAndRecord);
  testMethod(testLocalParticleOutsideALocalCellBecomesVirtual);
  testMethod(testVirtualParticleInsideALocalCellBecomesLocal);
  testMethod(testDeleteVirtualParticlesKeepsTheLocalOnes);
}

/** The recorded statistics carry the three counters and the numbers of the local particles. */
void exahype2::particles::tests::ParallelStateTest::testStatisticsClearAndRecord() {
  ParticleSet::clearTestState();

  exahype2::particles::ParallelStateStatistics statistics;
  statistics.numberOfRemainingLocalParticles          = 1;
  statistics.numberOfExpiredHaloParticles             = 2;
  statistics.numberOfParticlesThatHaveLeftTheirDomain = 3;
  statistics.remainingLocalParticleNumbers.insert({TreeId, 201});
  statistics.record<ParticleSet>();

  const auto& recorded = ParticleSet::getRecordedParticleStateStatistics();
  validateEquals(recorded[0], 1);
  validateEquals(recorded[1], 2);
  validateEquals(recorded[2], 3);
  validateEquals(static_cast<int>(ParticleSet::getRecordedLocalParticleNumbers().size()), 1);
  validateEquals(static_cast<int>(ParticleSet::getRecordedLocalParticleNumbers().count({TreeId, 201})), 1);

  statistics.clear();
  validateEquals(statistics.numberOfRemainingLocalParticles, 0);
  validateEquals(statistics.numberOfExpiredHaloParticles, 0);
  validateEquals(statistics.numberOfParticlesThatHaveLeftTheirDomain, 0);
  validateTrue(statistics.remainingLocalParticleNumbers.empty());

  ParticleSet::clearTestState();
}

/** A local particle that no local cell around its vertex contains becomes virtual. */
void exahype2::particles::tests::ParallelStateTest::testLocalParticleOutsideALocalCellBecomesVirtual() {
  ParticleSet::clearTestState();

  auto event = leafCellEvent();
  event.setIsAdjacentCellLocal(static_cast<std::uint32_t>(0));
  const peano4::dataview::VertexMarker marker(event, 0);

  ParticleSet fineGridVertexParticles;
  auto* const particle = createParticle(Particle::ParallelState::Local, 211);
  fineGridVertexParticles.addParticle(particle);
  exahype2::particles::tests::recordInitialParticleAssignment(*particle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::ParallelStateStatistics statistics;
  exahype2::particles::updateParallelStateOfParticles(
    marker,
    fineGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateTrue(particle->getParallelState() == Particle::ParallelState::Virtual);
  validateEquals(statistics.numberOfParticlesThatHaveLeftTheirDomain, 1);
  validateEquals(fineGridVertexParticles.size(), 1);

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}

/** A virtual particle that a local cell around its vertex contains becomes local. */
void exahype2::particles::tests::ParallelStateTest::testVirtualParticleInsideALocalCellBecomesLocal() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);

  ParticleSet fineGridVertexParticles;
  auto* const particle = createParticle(Particle::ParallelState::Virtual, 221);
  fineGridVertexParticles.addParticle(particle);
  exahype2::particles::tests::recordInitialParticleAssignment(*particle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::ParallelStateStatistics statistics;
  exahype2::particles::updateParallelStateOfParticles(
    marker,
    fineGridVertexParticles,
    "TestParticle",
    TreeId,
    statistics
  );

  validateTrue(particle->getParallelState() == Particle::ParallelState::Local);
  validateEquals(statistics.numberOfParticlesThatHaveLeftTheirDomain, 0);
  validateEquals(fineGridVertexParticles.size(), 1);

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}

/** The last vertex event counts every local particle and deletes every virtual one. */
void exahype2::particles::tests::ParallelStateTest::testDeleteVirtualParticlesKeepsTheLocalOnes() {
  ParticleSet::clearTestState();

  const peano4::dataview::VertexMarker marker(leafCellEvent(), 0);

  ParticleSet fineGridVertexParticles;
  auto* const localParticle   = createParticle(Particle::ParallelState::Local, 231);
  auto* const virtualParticle = createParticle(Particle::ParallelState::Virtual, 232);
  fineGridVertexParticles.addParticle(localParticle);
  fineGridVertexParticles.addParticle(virtualParticle);
  exahype2::particles::tests::recordInitialParticleAssignment(*virtualParticle, marker.getX(), marker.getH(), TreeId);

  exahype2::particles::ParallelStateStatistics statistics;
  exahype2::particles::deleteVirtualParticles(marker, fineGridVertexParticles, "TestParticle", TreeId, statistics);

  validateEquals(fineGridVertexParticles.size(), 1);
  validateEquals(*fineGridVertexParticles.begin(), localParticle);
  validateEquals(statistics.numberOfRemainingLocalParticles, 1);
  validateEquals(statistics.numberOfExpiredHaloParticles, 1);
  validateEquals(ParticleSet::getNumberOfDeletedParticles(), 1);
  validateEquals(static_cast<int>(statistics.remainingLocalParticleNumbers.size()), 1);
  validateEquals(static_cast<int>(statistics.remainingLocalParticleNumbers.count({TreeId, 231})), 1);

  fineGridVertexParticles.deleteParticles();
  ParticleSet::clearTestState();
}
