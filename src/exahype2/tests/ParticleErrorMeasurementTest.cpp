// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleErrorMeasurementTest.h"

#include "exahype2/ParticleErrorMeasurement.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  struct Particle final {
    enum class ParallelState { Local, Virtual };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    int                                   number[2]{0, 0};
    ParallelState                         parallelState{ParallelState::Local};

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    [[nodiscard]] int           getNumber(const int index) const { return number[index]; }
    [[nodiscard]] ParallelState getParallelState() const { return parallelState; }
  };

  struct Vertices final {
    std::vector<Particle*> particles[peano4::VerticesPerCell]{};

    const std::vector<Particle*>& operator()(const int vertex) const { return particles[vertex]; }
  };
} // namespace

exahype2::tests::ParticleErrorMeasurementTest::ParticleErrorMeasurementTest() :
  tarch::tests::TestCase("exahype2::tests::ParticleErrorMeasurementTest") {}

void exahype2::tests::ParticleErrorMeasurementTest::run() {
  testMethod(testCollectsEveryLocalParticleOnce);
  testMethod(testAccumulatesThePositionErrorOfEveryParticle);
}

/**
 * Checks that a particle two vertices hold appears once in the map and a virtual particle not
 * at all.
 */
void exahype2::tests::ParticleErrorMeasurementTest::testCollectsEveryLocalParticleOnce() {
  Particle first;
  first.number[1] = 1;
  Particle second;
  second.number[1] = 2;
  Particle remote;
  remote.number[1]     = 3;
  remote.parallelState = Particle::ParallelState::Virtual;

  Vertices vertices;
  vertices.particles[0] = {&first, &remote};
  vertices.particles[1] = {&first, &second};

  std::map<std::pair<int, int>, Particle> collected;
  exahype2::collectLocalParticles(vertices, collected);

  validateEquals(collected.size(), 2);
  validateEquals(collected.count(std::make_pair(0, 1)), 1);
  validateEquals(collected.count(std::make_pair(0, 2)), 1);
}

void exahype2::tests::ParticleErrorMeasurementTest::testAccumulatesThePositionErrorOfEveryParticle() {
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  std::map<std::pair<int, int>, Particle> particles;
  particles[std::make_pair(0, 1)].x(0) = 1.0;
  particles[std::make_pair(0, 2)].x(1) = 2.0;

  exahype2::ErrorMeasurement measurement;
  measurement.open((tarch::tests::getWritableTestDirectory() / "ParticleErrorMeasurementTest").string(), 0.0);
  exahype2::accumulateParticlePositionErrors<DIMENSIONS + 1>(
    particles,
    0.5,
    0.1,
    [&](const Particle* const particle, const double timeStamp, const double timeStepSize, double* const solution) {
      validateEquals(particle->getNumber(0), 0);
      validateNumericalEquals(timeStamp, 0.5);
      validateNumericalEquals(timeStepSize, 0.1);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        solution[axis] = 0.0;
      }
    },
    measurement
  );

  const auto norms = measurement.reduceAndRecord(0.5);
  validateNumericalEquals(norms[0], 3.0 * numberOfRanks);
  validateNumericalEquals(norms[1], std::sqrt(5.0 * numberOfRanks));
  validateNumericalEquals(norms[2], 2.0);
}
