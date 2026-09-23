// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TracingTest.h"

#include "exahype2/Tracing.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/la/Vector.h"

namespace {
  struct Particle final {
    enum class MoveState { NotMovedYet, Moved };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    MoveState                             moveState{MoveState::NotMovedYet};

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    void                    setX(const tarch::la::Vector<DIMENSIONS, double>& position) { x = position; }
    [[nodiscard]] MoveState getMoveState() const { return moveState; }
    void                    setMoveState(const MoveState state) { moveState = state; }
  };

  const tarch::la::Vector<DIMENSIONS, double> UnitDomainWidth(1.0);

  peano4::dataview::CellMarker createUnitCell() {
    peano4::dataview::CellMarker marker{};
    marker.level = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0;
      marker.centre[axis] = 0.5;
    }
    return marker;
  }
} // namespace

exahype2::tests::TracingTest::TracingTest() :
  tarch::tests::TestCase("exahype2::tests::TracingTest") {}

void exahype2::tests::TracingTest::run() {
  testMethod(testMovesOnlyTheContainedParticlesThatHaveNotMoved);
  testMethod(testMovesAParticleOntoItsPeriodicImage);
  testMethod(testMarksEveryParticleAsNotMoved);
}

/**
 * Checks that a cell projects the contained particle that has not moved, and leaves the
 * particle outside the cell and the particle another cell has moved alone.
 */
void exahype2::tests::TracingTest::testMovesOnlyTheContainedParticlesThatHaveNotMoved() {
  Particle inside;
  inside.x = tarch::la::Vector<DIMENSIONS, double>(0.25);
  Particle outside;
  outside.x = tarch::la::Vector<DIMENSIONS, double>(2.0);
  Particle moved;
  moved.x         = tarch::la::Vector<DIMENSIONS, double>(0.75);
  moved.moveState = Particle::MoveState::Moved;

  const std::list<Particle*> particles   = {&inside, &outside, &moved};
  int                        projections = 0;
  exahype2::moveParticlesThroughCell(
    createUnitCell(),
    particles,
    UnitDomainWidth,
    std::bitset<DIMENSIONS>{},
    [&](Particle* const particle) {
      validateEquals(particle, &inside);
      projections++;
    }
  );

  validateEquals(projections, 1);
  validateTrue(inside.moveState == Particle::MoveState::Moved);
  validateTrue(outside.moveState == Particle::MoveState::NotMovedYet);
  validateTrue(moved.moveState == Particle::MoveState::Moved);
}

/**
 * Checks that a cell moves a particle behind the wall of a periodic axis onto its image within
 * the cell and projects it, and leaves a particle behind the wall of a non-periodic axis alone.
 */
void exahype2::tests::TracingTest::testMovesAParticleOntoItsPeriodicImage() {
  Particle behindThePeriodicWall;
  behindThePeriodicWall.x    = tarch::la::Vector<DIMENSIONS, double>(0.25);
  behindThePeriodicWall.x(0) = 1.25;
  Particle behindTheSolidWall;
  behindTheSolidWall.x    = tarch::la::Vector<DIMENSIONS, double>(0.25);
  behindTheSolidWall.x(1) = 1.25;

  std::bitset<DIMENSIONS> periodicBoundaries{};
  periodicBoundaries.set(0);
  const std::list<Particle*> particles   = {&behindThePeriodicWall, &behindTheSolidWall};
  int                        projections = 0;
  exahype2::moveParticlesThroughCell(
    createUnitCell(),
    particles,
    UnitDomainWidth,
    periodicBoundaries,
    [&](Particle* const particle) {
      validateEquals(particle, &behindThePeriodicWall);
      projections++;
    }
  );

  validateEquals(projections, 1);
  validateNumericalEquals(behindThePeriodicWall.x(0), 0.25);
  validateTrue(behindThePeriodicWall.moveState == Particle::MoveState::Moved);
  validateNumericalEquals(behindTheSolidWall.x(1), 1.25);
  validateTrue(behindTheSolidWall.moveState == Particle::MoveState::NotMovedYet);
}

/**
 * Checks that every particle of a vertex returns to the not-moved state.
 */
void exahype2::tests::TracingTest::testMarksEveryParticleAsNotMoved() {
  Particle first;
  first.moveState = Particle::MoveState::Moved;
  Particle second;
  second.moveState                 = Particle::MoveState::Moved;
  std::vector<Particle*> particles = {&first, &second};

  exahype2::markParticlesAsNotMoved(particles);

  validateTrue(first.moveState == Particle::MoveState::NotMovedYet);
  validateTrue(second.moveState == Particle::MoveState::NotMovedYet);
}
