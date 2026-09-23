// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticlePlotterTest.h"

#include "exahype2/plotter/ParticlePlotter.h"
#include "exahype2/plotter/tests/RecordingFileWriter.h"
#include "tarch/la/Vector.h"

namespace {
  struct TestParticle final {
    enum class ParallelState : std::uint8_t { Local, Virtual };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    ParallelState                         parallelState{ParallelState::Local};
    double                                number{0.0};

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    [[nodiscard]] ParallelState                                getParallelState() const { return parallelState; }
    [[nodiscard]] double                                       getNumber() const { return number; }
  };

  struct TestParticleSet final {
    using DoFType = TestParticle;

    std::vector<TestParticle*> particles{};

    [[nodiscard]] std::vector<TestParticle*>::iterator begin() { return particles.begin(); }
    [[nodiscard]] std::vector<TestParticle*>::iterator end() { return particles.end(); }
  };

  TestParticle createParticle(
    const double                      position,
    const TestParticle::ParallelState parallelState,
    const double                      number
  ) {
    TestParticle particle;
    particle.x             = tarch::la::Vector<DIMENSIONS, double>(position);
    particle.parallelState = parallelState;
    particle.number        = number;
    return particle;
  }
} // namespace

exahype2::plotter::tests::ParticlePlotterTest::ParticlePlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::ParticlePlotterTest") {}

void exahype2::plotter::tests::ParticlePlotterTest::run() {
  testMethod(testPlotsEveryLocalParticleOfTheVertex);
  testMethod(testSkipsTheVertexOnAnUpperPeriodicWall);
}

/**
 * Checks that a vertex plots one point per local particle, skips the particles another rank
 * owns, and lets the caller write the attributes of each plotted particle.
 */
void exahype2::plotter::tests::ParticlePlotterTest::testPlotsEveryLocalParticleOfTheVertex() {
  TestParticle first  = createParticle(0.25, TestParticle::ParallelState::Local, 11.0);
  TestParticle second = createParticle(0.5, TestParticle::ParallelState::Virtual, 22.0);
  TestParticle third  = createParticle(0.75, TestParticle::ParallelState::Local, 33.0);

  TestParticleSet particles;
  particles.particles = {&first, &second, &third};

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  ParticlePlotter plotter;
  plotter.open(snapshot, 8, {{"Number", 1}});

  const peano4::dataview::VertexMarker marker{};
  plotter.plotParticles(
    marker,
    particles,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>{},
    [&](const int cellIndex, const auto* const particle) {
      plotter.getAttributePlotter(0).plotCellData(cellIndex, particle->getNumber());
    }
  );

  const auto& writer = getRecordingFileWriter(snapshot);
  validateEquals(writer.getNumberOfPoints(0), 2);
  validateEquals(writer.getNumberOfCells(0), 2);
  validateNumericalEquals(writer.getPointCoordinate(0, 0, 0), 0.25);
  validateNumericalEquals(writer.getPointCoordinate(0, 1, 0), 0.75);

  const std::vector<double> numbers = writer.getCellField(0, "Number");
  validateEquals(static_cast<int>(numbers.size()), 2);
  validateNumericalEquals(numbers[0], 11.0);
  validateNumericalEquals(numbers[1], 33.0);
  validateNumericalEquals(writer.getCellField(0, "Subtree")[0], 8.0);
  validateNumericalEquals(writer.getCellField(0, "RefinementLevel")[0], 1.0);
}

/**
 * Checks that isAtUpperPeriodicBoundary() holds only for a vertex on an upper wall of a periodic
 * axis and that plotParticles() plots no particle of such a vertex.
 */
void exahype2::plotter::tests::ParticlePlotterTest::testSkipsTheVertexOnAnUpperPeriodicWall() {
  TestParticle    particle = createParticle(0.0, TestParticle::ParallelState::Local, 11.0);
  TestParticleSet particles;
  particles.particles = {&particle};

  const peano4::dataview::VertexMarker        marker{};
  const tarch::la::Vector<DIMENSIONS, double> offsetOfWallVertex(-1.0);
  const tarch::la::Vector<DIMENSIONS, double> widthToWallVertex(1.0);
  const tarch::la::Vector<DIMENSIONS, double> widthPastWallVertex(2.0);

  std::bitset<DIMENSIONS> periodicBoundaries;
  periodicBoundaries.set();
  validateTrue(isAtUpperPeriodicBoundary(marker, periodicBoundaries, offsetOfWallVertex, widthToWallVertex));
  validateTrue(not isAtUpperPeriodicBoundary(marker, periodicBoundaries, offsetOfWallVertex, widthPastWallVertex));

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  ParticlePlotter plotter;
  plotter.open(snapshot, 0, {});
  plotter.plotParticles(
    marker,
    particles,
    offsetOfWallVertex,
    widthToWallVertex,
    periodicBoundaries,
    []([[maybe_unused]] const int cellIndex, [[maybe_unused]] const auto* const plottedParticle) {}
  );
  validateEquals(getRecordingFileWriter(snapshot).getNumberOfPoints(0), 0);

  periodicBoundaries.reset();
  validateTrue(not isAtUpperPeriodicBoundary(marker, periodicBoundaries, offsetOfWallVertex, widthToWallVertex));
}
