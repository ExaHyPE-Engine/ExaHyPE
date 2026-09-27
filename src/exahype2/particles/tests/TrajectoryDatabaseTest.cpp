// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryDatabaseTest.h"

#include "exahype2/particles/TrajectoryDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::string getTestFileBaseName(const std::string& name) {
    return (tarch::tests::getWritableTestDirectory() / name).string();
  }

  /**
   * Returns the file name TrajectoryDatabase writes for the calling rank.
   */
  std::string getOutputFileName(const std::string& baseName) {
    if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
      return baseName + "-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank()) + ".csv";
    }
    return baseName + ".csv";
  }

  std::vector<std::string> readNonEmptyLines(const std::string& fileName) {
    std::vector<std::string> lines;
    std::ifstream            file(fileName);
    std::string              line;
    while (std::getline(file, line)) {
      if (not line.empty()) {
        lines.push_back(line);
      }
    }
    return lines;
  }

  std::vector<double> parseRow(const std::string& line) {
    std::vector<double> values;
    std::istringstream  stream(line);
    std::string         cell;
    while (std::getline(stream, cell, ',')) {
      values.push_back(std::stod(cell));
    }
    return values;
  }

  tarch::la::Vector<DIMENSIONS, double> getMovingPosition(const int step, const double offset) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      x(axis) = offset + static_cast<double>(step) * (axis + 1);
    }
    return x;
  }
} // namespace

exahype2::particles::tests::TrajectoryDatabaseTest::TrajectoryDatabaseTest() :
  TestCase("exahype2::particles::tests::TrajectoryDatabaseTest") {}

void
exahype2::particles::tests::TrajectoryDatabaseTest::run() {
  testMethod(testAppendsChronologicallyWithoutDuplication);
  testMethod(testClearRewritesFileFromScratch);
  testMethod(testASnapshotIsDueForANewParticleAfterTheDeltasAndOnAMove);
}

void
exahype2::particles::tests::TrajectoryDatabaseTest::testAppendsChronologicallyWithoutDuplication() {
  const std::string         baseName = getTestFileBaseName("TrajectoryDatabaseTest_append");
  const std::string         fileName = getOutputFileName(baseName);
  const std::pair<int, int> firstParticle(1, 0);
  const std::pair<int, int> secondParticle(2, 0);
  std::filesystem::remove(fileName);

  {
    exahype2::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);
    for (int step = 1; step <= 3; step++) {
      database.addParticleSnapshot(firstParticle, static_cast<double>(step), getMovingPosition(step, 0.0));
    }
    for (int step = 1; step <= 2; step++) {
      database.addParticleSnapshot(secondParticle, static_cast<double>(step), getMovingPosition(step, 100.0));
    }

    const std::vector<std::string> lines = readNonEmptyLines(fileName);
    validateEqualsWithParams1(static_cast<int>(lines.size()), 6, fileName);

    int firstParticleRows  = 0;
    int secondParticleRows = 0;
    for (std::size_t lineIndex = 1; lineIndex < lines.size(); lineIndex++) {
      const std::vector<double> values = parseRow(lines[lineIndex]);
      if (not values.empty() and static_cast<int>(values[0]) == 1) {
        firstParticleRows++;
      } else if (not values.empty() and static_cast<int>(values[0]) == 2) {
        secondParticleRows++;
      }
    }
    validateEquals(firstParticleRows, 3);
    validateEquals(secondParticleRows, 2);
  }
  std::filesystem::remove(fileName);
}

void
exahype2::particles::tests::TrajectoryDatabaseTest::testClearRewritesFileFromScratch() {
  const std::string         baseName = getTestFileBaseName("TrajectoryDatabaseTest_clear");
  const std::string         fileName = getOutputFileName(baseName);
  const std::pair<int, int> particle(4, 2);
  std::filesystem::remove(fileName);

  {
    exahype2::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);
    for (int step = 1; step <= 2; step++) {
      database.addParticleSnapshot(particle, static_cast<double>(step), getMovingPosition(step, 0.0));
    }
    validateEqualsWithParams1(static_cast<int>(readNonEmptyLines(fileName).size()), 3, fileName);

    database.clear();
    database.addParticleSnapshot(particle, 5.0, getMovingPosition(9, 0.0));

    const std::vector<std::string> lines = readNonEmptyLines(fileName);
    validateEqualsWithParams1(static_cast<int>(lines.size()), 2, fileName);
    if (lines.size() == 2) {
      const std::vector<double> values = parseRow(lines[1]);
      validateEquals(static_cast<int>(values.size()), 3 + DIMENSIONS);
      validateNumericalEquals(values[2], 5.0);
    }
  }
  std::filesystem::remove(fileName);
}

/**
 * isSnapshotDue() holds for a particle the database lacks, after the time delta, on a move and on a
 * second position at the same time stamp, and it agrees with the snapshots addParticleSnapshot() stores.
 */
void
exahype2::particles::tests::TrajectoryDatabaseTest::testASnapshotIsDueForANewParticleAfterTheDeltasAndOnAMove() {
  const std::string         baseName = getTestFileBaseName("TrajectoryDatabaseTest_due");
  const std::string         fileName = getOutputFileName(baseName);
  const std::pair<int, int> particle(3, 0);
  std::filesystem::remove(fileName);
  {
    exahype2::particles::TrajectoryDatabase database(1e-8, 1.0);
    database.setOutputFileName(baseName);
    const tarch::la::Vector<DIMENSIONS, double> position(0.5);
    const tarch::la::Vector<DIMENSIONS, double> moved(0.75);

    validate(database.isSnapshotDue(particle, position, 0.0));
    database.addParticleSnapshot(particle, 0.0, position);
    validate(not database.isSnapshotDue(particle, position, 0.0));
    validate(database.isSnapshotDue(particle, moved, 0.0));
    validate(not database.isSnapshotDue(particle, position, 0.5));
    validate(database.isSnapshotDue(particle, moved, 0.5));
    validate(database.isSnapshotDue(particle, position, 1.0));

    database.addParticleSnapshot(particle, 0.5, position);
    database.addParticleSnapshot(particle, 1.0, position);
    validateEqualsWithParams1(static_cast<int>(readNonEmptyLines(fileName).size()), 3, fileName);
  }
  std::filesystem::remove(fileName);
}
