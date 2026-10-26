// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/particles/TrajectoryDatabase.h"

#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

namespace {
  std::string getTestFileBaseName(const std::string& name) {
    return (tarch::testing::getWritableTestDirectory() / name).string();
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

UNIT_TEST("The database appends every snapshot of every particle to its file once") {
  GIVEN("A database that writes into a new file of this rank and two particles") {
    const std::string         baseName = getTestFileBaseName("TrajectoryDatabaseTest_append");
    const std::string         fileName = getOutputFileName(baseName);
    const std::pair<int, int> firstParticle(1, 0);
    const std::pair<int, int> secondParticle(2, 0);
    std::filesystem::remove(fileName);
    exahype2::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    WHEN("The database adds three snapshots of the first moving particle and two of the second one") {
      for (int step = 1; step <= 3; step++) {
        database.addParticleSnapshot(firstParticle, static_cast<double>(step), getMovingPosition(step, 0.0));
      }
      for (int step = 1; step <= 2; step++) {
        database.addParticleSnapshot(secondParticle, static_cast<double>(step), getMovingPosition(step, 100.0));
      }
      const std::vector<std::string> lines              = readNonEmptyLines(fileName);
      int                            firstParticleRows  = 0;
      int                            secondParticleRows = 0;
      for (std::size_t lineIndex = 1; lineIndex < lines.size(); lineIndex++) {
        const std::vector<double> values = parseRow(lines[lineIndex]);
        if (not values.empty() and static_cast<int>(values[0]) == 1) {
          firstParticleRows++;
        } else if (not values.empty() and static_cast<int>(values[0]) == 2) {
          secondParticleRows++;
        }
      }

      THEN("The file contains the header and one row per snapshot, three of the first and two of the second particle") {
        VALIDATE_EQUALS(static_cast<int>(lines.size()), 6, fileName);
        VALIDATE_EQUALS(firstParticleRows, 3);
        VALIDATE_EQUALS(secondParticleRows, 2);
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A cleared database writes its file from scratch") {
  GIVEN("A database that writes into a new file of this rank and one particle") {
    const std::string         baseName = getTestFileBaseName("TrajectoryDatabaseTest_clear");
    const std::string         fileName = getOutputFileName(baseName);
    const std::pair<int, int> particle(4, 2);
    std::filesystem::remove(fileName);
    exahype2::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    WHEN("The database adds two snapshots of the moving particle") {
      for (int step = 1; step <= 2; step++) {
        database.addParticleSnapshot(particle, static_cast<double>(step), getMovingPosition(step, 0.0));
      }
      const int numberOfLines = static_cast<int>(readNonEmptyLines(fileName).size());

      THEN("The file contains the header and two rows") { VALIDATE_EQUALS(numberOfLines, 3, fileName); }

      AND_WHEN("The database clears and adds one snapshot at the time five") {
        database.clear();
        database.addParticleSnapshot(particle, 5.0, getMovingPosition(9, 0.0));
        const std::vector<std::string> lines  = readNonEmptyLines(fileName);
        const std::vector<double>      values = lines.size() == 2 ? parseRow(lines[1]) : std::vector<double>{};

        THEN("The file contains the header and the one row of the time five") {
          VALIDATE_EQUALS(static_cast<int>(lines.size()), 2, fileName);
          VALIDATE_EQUALS(static_cast<int>(values.size()), 3 + DIMENSIONS);
          VALIDATE_NUMERICAL_EQUALS(values[2], 5.0);
        }
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A snapshot is due for a new particle, on a move and after the time delta and the database stores it") {
  GIVEN("A database with the time delta one, a particle, its position and a moved position") {
    const std::string                           baseName = getTestFileBaseName("TrajectoryDatabaseTest_due");
    const std::string                           fileName = getOutputFileName(baseName);
    const std::pair<int, int>                   particle(3, 0);
    const tarch::la::Vector<DIMENSIONS, double> position(0.5);
    const tarch::la::Vector<DIMENSIONS, double> moved(0.75);
    std::filesystem::remove(fileName);
    exahype2::particles::TrajectoryDatabase database(1e-8, 1.0);
    database.setOutputFileName(baseName);

    THEN("A snapshot of the particle is due before the database stores its first snapshot") {
      VALIDATE(database.isSnapshotDue(particle, position, 0.0));
    }

    WHEN("The database stores a snapshot of the particle at the time zero") {
      database.addParticleSnapshot(particle, 0.0, position);

      THEN("A snapshot is due only on a move or after the time delta") {
        VALIDATE(not database.isSnapshotDue(particle, position, 0.0));
        VALIDATE(database.isSnapshotDue(particle, moved, 0.0));
        VALIDATE(not database.isSnapshotDue(particle, position, 0.5));
        VALIDATE(database.isSnapshotDue(particle, moved, 0.5));
        VALIDATE(database.isSnapshotDue(particle, position, 1.0));
      }

      AND_WHEN("The database adds snapshots of the particle at its position at the times one half and one") {
        database.addParticleSnapshot(particle, 0.5, position);
        database.addParticleSnapshot(particle, 1.0, position);
        const int numberOfLines = static_cast<int>(readNonEmptyLines(fileName).size());

        THEN("The file contains the header and the snapshots of the times zero and one") {
          VALIDATE_EQUALS(numberOfLines, 3, fileName);
        }
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("Under the rule EveryLaterTimestamp a snapshot of a particle at rest is due at every later time stamp") {
  GIVEN("A database with the time delta one and a snapshot of a particle at rest at the time zero") {
    const std::string                           baseName = getTestFileBaseName("TrajectoryDatabaseTest_later");
    const std::string                           fileName = getOutputFileName(baseName);
    const std::pair<int, int>                   particle(3, 0);
    const tarch::la::Vector<DIMENSIONS, double> position(0.5);
    std::filesystem::remove(fileName);
    exahype2::particles::TrajectoryDatabase database(1e-8, 1.0);
    database.setOutputFileName(baseName);
    database.addParticleSnapshot(particle, 0.0, position);
    const bool isDueWithinTheDelta = database.isSnapshotDue(particle, position, 0.5);

    WHEN("The database switches to the rule EveryLaterTimestamp") {
      database.setSnapshotRule(tarch::database::SnapshotRule::EveryLaterTimestamp);
      const bool isDueAtALaterTime    = database.isSnapshotDue(particle, position, 0.5);
      const bool isDueAtTheStoredTime = database.isSnapshotDue(particle, position, 0.0);

      THEN("A snapshot is due at the later time within the time delta but not at the stored time") {
        VALIDATE(not isDueWithinTheDelta);
        VALIDATE(isDueAtALaterTime);
        VALIDATE(not isDueAtTheStoredTime);
      }

      AND_WHEN("The database adds the snapshot of the time one half") {
        database.addParticleSnapshot(particle, 0.5, position);
        const bool isDueAtTheAddedTime = database.isSnapshotDue(particle, position, 0.5);
        const int  numberOfLines       = static_cast<int>(readNonEmptyLines(fileName).size());

        THEN("isSnapshotDue() returns false at that time and the file contains the header and both snapshots") {
          VALIDATE(not isDueAtTheAddedTime);
          VALIDATE_EQUALS(numberOfLines, 3, fileName);
        }
      }
    }
    std::filesystem::remove(fileName);
  }
}
