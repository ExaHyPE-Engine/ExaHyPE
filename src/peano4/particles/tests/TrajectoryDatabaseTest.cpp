// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryDatabaseTest.h"

#include "peano4/particles/TrajectoryDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::string testFileBaseName(const std::string& name) {
    return (tarch::tests::getWritableTestDirectory() / name).string();
  }

  /**
   * Returns the file name TrajectoryDatabase writes for the calling rank.
   */
  std::string outputFileName(const std::string& baseName) {
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

  tarch::la::Vector<DIMENSIONS, double> movingPosition(const int step, const double offset) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      x(axis) = offset + static_cast<double>(step) * (axis + 1);
    }
    return x;
  }
} // namespace

peano4::particles::tests::TrajectoryDatabaseTest::TrajectoryDatabaseTest() :
  TestCase("peano4::particles::tests::TrajectoryDatabaseTest") {}

void peano4::particles::tests::TrajectoryDatabaseTest::run() {
  testMethod(testAppendsChronologicallyWithoutDuplication);
  testMethod(testClearRewritesFileFromScratch);
}

void peano4::particles::tests::TrajectoryDatabaseTest::testAppendsChronologicallyWithoutDuplication() {
  const std::string         baseName = testFileBaseName("TrajectoryDatabaseTest_append");
  const std::string         fileName = outputFileName(baseName);
  const std::pair<int, int> firstParticle(1, 0);
  const std::pair<int, int> secondParticle(2, 0);
  std::filesystem::remove(fileName);

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);
    for (int step = 1; step <= 3; step++) {
      database.addParticleSnapshot(firstParticle, static_cast<double>(step), movingPosition(step, 0.0));
    }
    for (int step = 1; step <= 2; step++) {
      database.addParticleSnapshot(secondParticle, static_cast<double>(step), movingPosition(step, 100.0));
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

void peano4::particles::tests::TrajectoryDatabaseTest::testClearRewritesFileFromScratch() {
  const std::string         baseName = testFileBaseName("TrajectoryDatabaseTest_clear");
  const std::string         fileName = outputFileName(baseName);
  const std::pair<int, int> particle(4, 2);
  std::filesystem::remove(fileName);

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);
    for (int step = 1; step <= 2; step++) {
      database.addParticleSnapshot(particle, static_cast<double>(step), movingPosition(step, 0.0));
    }
    validateEqualsWithParams1(static_cast<int>(readNonEmptyLines(fileName).size()), 3, fileName);

    database.clear();
    database.addParticleSnapshot(particle, 5.0, movingPosition(9, 0.0));

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
