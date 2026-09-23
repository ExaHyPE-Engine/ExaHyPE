// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryRecordingTest.h"

#include "exahype2/particles/TrajectoryRecording.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/TrajectoryDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int    DataEntriesPerParticle = 2;
  constexpr double CellWidth              = 0.2;
  constexpr double CellCentre             = 0.5;
  constexpr double TimeStamp              = 3.5;

  /** Stands in for a generated tracer particle. */
  struct TracerParticle final {
    enum class ParallelState { Local, Virtual };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    int                                   number[2]{0, 0};
    ParallelState                         parallelState{ParallelState::Local};
    std::vector<double>                   data = std::vector<double>(DataEntriesPerParticle, 0.0);

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    [[nodiscard]] int                        getNumber(const int index) const { return number[index]; }
    [[nodiscard]] ParallelState              getParallelState() const { return parallelState; }
    [[nodiscard]] const std::vector<double>& getData() const { return data; }
  };

  /** Stands in for the generated particle container of the vertices around one cell. */
  struct Vertices final {
    std::vector<TracerParticle*> particles[peano4::VerticesPerCell]{};

    const std::vector<TracerParticle*>& operator()(const int vertex) const { return particles[vertex]; }
  };

  peano4::dataview::CellMarker createCellMarker() {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(CellCentre);
    return marker;
  }

  TracerParticle createParticle(const double position, const int number, const double firstDataValue) {
    TracerParticle particle;
    particle.x         = tarch::la::Vector<DIMENSIONS, double>(position);
    particle.number[0] = 7;
    particle.number[1] = number;
    particle.data[0]   = firstDataValue;
    particle.data[1]   = firstDataValue + 1.0;
    return particle;
  }

  std::string testFileBaseName(const std::string& name) {
    return (tarch::tests::getWritableTestDirectory() / name).string();
  }

  /** Returns the file name TrajectoryDatabase writes for the calling rank. */
  std::string outputFileName(const std::string& baseName) {
    if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
      return baseName + "-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank()) + ".csv";
    }
    return baseName + ".csv";
  }

  /** Returns the rows behind the header, each split into its values. */
  std::vector<std::vector<double>> readDataRows(const std::string& fileName) {
    std::vector<std::vector<double>> rows;
    std::ifstream                    file(fileName);
    std::string                      line;
    bool                             isHeader = true;
    while (std::getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      if (isHeader) {
        isHeader = false;
        continue;
      }
      std::vector<double> values;
      std::istringstream  stream(line);
      std::string         cell;
      while (std::getline(stream, cell, ',')) {
        values.push_back(std::stod(cell));
      }
      rows.push_back(values);
    }
    return rows;
  }
} // namespace

exahype2::particles::tests::TrajectoryRecordingTest::TrajectoryRecordingTest() :
  tarch::tests::TestCase("exahype2::particles::tests::TrajectoryRecordingTest") {}

void exahype2::particles::tests::TrajectoryRecordingTest::run() {
  testMethod(testRecordsEveryLocalParticleTheCellOwns);
  testMethod(testTimeStampOutsideTheRecordWindowAddsNoSnapshot);
}

/**
 * A local particle inside the cell reaches the database with its numbers, its time stamp, its
 * position and its data, while a virtual particle and a particle outside the cell do not.
 */
void exahype2::particles::tests::TrajectoryRecordingTest::testRecordsEveryLocalParticleTheCellOwns() {
  const std::string baseName = testFileBaseName("TrajectoryRecordingTest_records");
  const std::string fileName = outputFileName(baseName);
  std::filesystem::remove(fileName);

  TracerParticle firstLocalParticle     = createParticle(0.45, 1, 1.5);
  TracerParticle secondLocalParticle    = createParticle(0.55, 2, 3.5);
  TracerParticle virtualParticle        = createParticle(0.50, 3, 5.5);
  virtualParticle.parallelState         = TracerParticle::ParallelState::Virtual;
  TracerParticle particleOutsideTheCell = createParticle(0.90, 4, 7.5);

  Vertices vertices;
  vertices.particles[0] = {&firstLocalParticle, &virtualParticle};
  vertices.particles[1] = {&secondLocalParticle, &particleOutsideTheCell};

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    exahype2::particles::recordParticleSnapshots<TracerParticle>(
      createCellMarker(),
      vertices,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      TimeStamp,
      0.0,
      10.0,
      database
    );

    const std::vector<std::vector<double>> rows = readDataRows(fileName);
    validateEqualsWithParams1(static_cast<int>(rows.size()), 2, fileName);
    for (const std::vector<double>& row : rows) {
      validateEquals(static_cast<int>(row.size()), 3 + DIMENSIONS + DataEntriesPerParticle);
      validateNumericalEquals(row[0], 7.0);
      validateNumericalEquals(row[2], TimeStamp);
      const bool isFirstParticle = static_cast<int>(row[1]) == 1;
      validateTrue(isFirstParticle or static_cast<int>(row[1]) == 2);
      const double expectedPosition = isFirstParticle ? 0.45 : 0.55;
      const double expectedData     = isFirstParticle ? 1.5 : 3.5;
      validateNumericalEquals(row[3], expectedPosition);
      validateNumericalEquals(row[3 + DIMENSIONS], expectedData);
      validateNumericalEquals(row[4 + DIMENSIONS], expectedData + 1.0);
    }
  }
  std::filesystem::remove(fileName);
}

/** A time stamp above the record window leaves the database without a single snapshot. */
void exahype2::particles::tests::TrajectoryRecordingTest::testTimeStampOutsideTheRecordWindowAddsNoSnapshot() {
  const std::string baseName = testFileBaseName("TrajectoryRecordingTest_window");
  const std::string fileName = outputFileName(baseName);
  std::filesystem::remove(fileName);

  TracerParticle particle = createParticle(0.45, 1, 1.5);

  Vertices vertices;
  vertices.particles[0] = {&particle};

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    exahype2::particles::recordParticleSnapshots<TracerParticle>(
      createCellMarker(),
      vertices,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      TimeStamp,
      0.0,
      1.0,
      database
    );

    validateFalse(std::filesystem::exists(fileName));
  }
  std::filesystem::remove(fileName);
}
