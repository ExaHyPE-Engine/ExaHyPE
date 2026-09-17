// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticlesTest.h"

#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/ParticleSet.h"
#include "peano4/particles/particles.h"
#include "peano4/particles/tests/TestParticle.h"

namespace {
  struct FactoryParticle final {
    enum class ParallelState { Local, Virtual };

#if PEANO_DEBUG
    void setDebugX(const tarch::la::Vector<DIMENSIONS, double>& position) { debugX = position; }
    void setDebugH(const double cellSize) { debugH = cellSize; }
#endif

    void setX(const tarch::la::Vector<DIMENSIONS, double>& position) { x = position; }
    void setSearchRadius(const double radius) { searchRadius = radius; }
    void setData(const int index, const double value) { data[index] = value; }
    void setParallelState(const ParallelState state) { parallelState = state; }

    const std::array<double, 2>& getData() const { return data; }

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    double                                searchRadius{0.0};
    std::array<double, 2>                 data{};
    ParallelState                         parallelState{ParallelState::Local};
#if PEANO_DEBUG
    tarch::la::Vector<DIMENSIONS, double> debugX{0.0};
    double                                debugH{0.0};
#endif
  };
} // namespace

peano4::particles::tests::ParticlesTest::ParticlesTest() :
  TestCase("peano4::particles::tests::ParticlesTest") {}

void peano4::particles::tests::ParticlesTest::run() {
  testMethod(testPeriodicImageTowards);
  testMethod(testApplyPeriodicBoundaryConditions);
  testMethod(testParticleFactory);
  testMethod(testParticleSetStatistics);
}

/** The image of a position lies within half the domain width of the reference on every periodic axis. */
void peano4::particles::tests::ParticlesTest::testPeriodicImageTowards() {
  const tarch::la::Vector<DIMENSIONS, double> domainWidth(2.0);
  const tarch::la::Vector<DIMENSIONS, double> lowerWall(0.0);
  const tarch::la::Vector<DIMENSIONS, double> upperWall(2.0);
  const tarch::la::Vector<DIMENSIONS, double> nearUpperWall(1.9);
  const tarch::la::Vector<DIMENSIONS, double> nearLowerWall(0.1);
  std::bitset<DIMENSIONS>                     firstAxisPeriodic;
  firstAxisPeriodic[0] = true;

  const auto unchanged =
    peano4::particles::periodicImageTowards(nearUpperWall, lowerWall, domainWidth, std::bitset<DIMENSIONS>());
  validateNumericalEquals(unchanged(0), 1.9);

  const auto belowLowerWall =
    peano4::particles::periodicImageTowards(nearUpperWall, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(belowLowerWall(0), -0.1);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(belowLowerWall(axis), 1.9);
  }

  const auto aboveUpperWall =
    peano4::particles::periodicImageTowards(nearLowerWall, upperWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(aboveUpperWall(0), 2.1);

  const auto nearReference =
    peano4::particles::periodicImageTowards(nearLowerWall, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(nearReference(0), 0.1);

  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  const auto halfWidthAway = peano4::particles::periodicImageTowards(centre, lowerWall, domainWidth, firstAxisPeriodic);
  validateNumericalEquals(halfWidthAway(0), 1.0);

  // The test driver initialises a domain without a periodic axis.
  validateFalse(peano4::particles::domainHasPeriodicAxes());
  const auto withinTheDomain = peano4::particles::periodicImageTowards(nearUpperWall, lowerWall);
  validateNumericalEquals(withinTheDomain(0), 1.9);

  peano4::dataview::CellMarker cell{};
  cell.h                 = tarch::la::Vector<DIMENSIONS, double>(0.5);
  cell.centre            = tarch::la::Vector<DIMENSIONS, double>(0.25);
  const auto towardsCell = peano4::particles::periodicImageTowardsCell(nearUpperWall, cell);
  validateNumericalEquals(towardsCell(0), 1.9);
}

/** A position outside the domain moves back by one domain width on a periodic axis only. */
void peano4::particles::tests::ParticlesTest::testApplyPeriodicBoundaryConditions() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(-1.0);
  const tarch::la::Vector<DIMENSIONS, double> domainSize(2.0);
  const tarch::la::Vector<DIMENSIONS, double> beyondUpperWall(1.25);
  const tarch::la::Vector<DIMENSIONS, double> beyondLowerWall(-1.25);
  std::bitset<DIMENSIONS>                     firstAxisPeriodic;
  firstAxisPeriodic[0] = true;

  const auto wrappedDown =
    peano4::particles::applyPeriodicBoundaryConditions(beyondUpperWall, domainOffset, domainSize, firstAxisPeriodic);
  validateNumericalEquals(wrappedDown(0), -0.75);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(wrappedDown(axis), 1.25);
  }

  const auto wrappedUp =
    peano4::particles::applyPeriodicBoundaryConditions(beyondLowerWall, domainOffset, domainSize, firstAxisPeriodic);
  validateNumericalEquals(wrappedUp(0), 0.75);

  const auto inside = peano4::particles::applyPeriodicBoundaryConditions(
    tarch::la::Vector<DIMENSIONS, double>(0.5),
    domainOffset,
    domainSize,
    firstAxisPeriodic
  );
  validateNumericalEquals(inside(0), 0.5);
}

void peano4::particles::tests::ParticlesTest::testParticleFactory() {
  FactoryParticle                             particle;
  const tarch::la::Vector<DIMENSIONS, double> position(0.25);
  peano4::particles::initWithData(particle, position, 0.5, {1.0});
  validateEquals(particle.x, position);
  validateNumericalEquals(particle.searchRadius, 0.5);
  validateNumericalEquals(particle.data[0], 1.0);
  validateNumericalEquals(particle.data[1], 0.0);
  validate(particle.parallelState == FactoryParticle::ParallelState::Virtual);

  const tarch::la::Vector<DIMENSIONS, double> unitCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> unitSize(1.0);
  auto                                        localParticles =
    peano4::particles::createEquallySpacedParticles<FactoryParticle>(0.5, unitCentre, unitSize, false);
  validateEquals(localParticles.size(), static_cast<std::size_t>(TWO_POWER_D));
  for (auto* const localParticle : localParticles) {
    delete localParticle;
  }

  auto alignedParticles = peano4::particles::createParticlesAlignedWithGlobalCartesianMesh<FactoryParticle>(
    0.5,
    unitCentre,
    unitSize,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    unitSize,
    false
  );
  validateEquals(alignedParticles.size(), static_cast<std::size_t>(TWO_POWER_D));
  for (auto* const alignedParticle : alignedParticles) {
    delete alignedParticle;
  }
}

void peano4::particles::tests::ParticlesTest::testParticleSetStatistics() {
  using TestParticleSet = peano4::particles::ParticleSet<peano4::particles::tests::TestParticle>;

  TestParticleSet::clearReassignmentStatistics();
  validate(not TestParticleSet::registeredAnyResorting());
  TestParticleSet::updateLiftDropStatistics(1, 2, 3, 4, 5, 6);
  validate(TestParticleSet::registeredAnyResorting());
  validateEquals(TestParticleSet::getNumberOfDropsIntoHorizontalTreeDecomposition(), 6);
  validate(TestParticleSet::printReassignmentStatistics().find("#lifts=1") != std::string::npos);

  TestParticleSet::clearParticleStateStatistics();
  TestParticleSet::updateNumberOfLocalAndExpiredParticles(7, 2, 1, {{3, 4}, {3, 5}});
  validateEquals(TestParticleSet::getNumberOfRemainingLocalParticles(), 7);
  validate(TestParticleSet::printParticleStateStatistics().find("#remaining=7") != std::string::npos);

  TestParticleSet particleSet;
#if PEANO_DEBUG
  const tarch::la::Vector<DIMENSIONS, double> debugX(0.25);
  const tarch::la::Vector<DIMENSIONS, double> debugH(0.5);
  particleSet.setDebugX(debugX);
  particleSet.setDebugH(debugH);
  validateEquals(particleSet.getDebugX(), debugX);
  validateEquals(particleSet.getDebugH(), debugH);
#endif
  validate(not particleSet.toString().empty());

  TestParticleSet::clearReassignmentStatistics();
  TestParticleSet::clearParticleStateStatistics();
}
