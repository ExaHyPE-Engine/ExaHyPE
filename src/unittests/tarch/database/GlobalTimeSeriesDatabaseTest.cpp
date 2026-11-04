// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/database/GlobalTimeSeriesDatabase.h"

#include "tarch/database/SnapshotDatabase.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Returns the base name of a file of this rank in the writable test directory. Two ranks never
   * write the same file.
   */
  std::string getTestFileBaseName(const std::string& name) {
    const std::string rank = std::to_string(tarch::mpi::Rank::getInstance().getRank());
    return (tarch::testing::getWritableTestDirectory() / (name + "_" + rank)).string();
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
} // namespace

UNIT_TEST("Every added snapshot reaches the file once and in the order of its time stamps") {
  GIVEN("A database that writes into a fresh file") {
    const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_append");
    const std::string fileName = baseName + ".csv";
    std::filesystem::remove(fileName);
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);

    WHEN("The database adds four snapshots of two values at the time stamps one to four") {
      for (int step = 1; step <= 4; step++) {
        const double data[2] = {10.0 * step, 20.0 * step};
        database.addGlobalSnapshot(static_cast<double>(step), 2, data);
      }
      const std::vector<std::string> lines = readNonEmptyLines(fileName);

      THEN("The file contains the header and one row per snapshot with its time stamp and its two values") {
        VALIDATE_EQUALS(static_cast<int>(lines.size()), 5, fileName);
        for (int row = 0; row < 4; row++) {
          const std::vector<double> values = parseRow(lines[row + 1]);
          VALIDATE_EQUALS(static_cast<int>(values.size()), 3);
          VALIDATE_NUMERICAL_EQUALS(values[0], static_cast<double>(row + 1));
          VALIDATE_NUMERICAL_EQUALS(values[1], 10.0 * (row + 1));
          VALIDATE_NUMERICAL_EQUALS(values[2], 20.0 * (row + 1));
        }
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A cleared database rewrites its file with the snapshots added after the clear only") {
  GIVEN("A database that writes into a fresh file") {
    const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_clear");
    const std::string fileName = baseName + ".csv";
    std::filesystem::remove(fileName);
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);

    WHEN("The database adds the value one at the time stamps one and two") {
      const double firstData[1] = {1.0};
      database.addGlobalSnapshot(1.0, 1, firstData);
      database.addGlobalSnapshot(2.0, 1, firstData);
      const std::vector<std::string> linesBeforeTheClear = readNonEmptyLines(fileName);

      THEN("The file contains the header and two rows") {
        VALIDATE_EQUALS(static_cast<int>(linesBeforeTheClear.size()), 3, fileName);
      }

      AND_WHEN("The database clears and adds the value five at the time stamp three") {
        database.clear();
        const double secondData[1] = {5.0};
        database.addGlobalSnapshot(3.0, 1, secondData);
        const std::vector<std::string> lines = readNonEmptyLines(fileName);

        THEN("The file contains the header and the row of the time stamp three with the value five") {
          VALIDATE_EQUALS(static_cast<int>(lines.size()), 2, fileName);
          const std::vector<double> values = parseRow(lines[1]);
          VALIDATE_EQUALS(static_cast<int>(values.size()), 2);
          VALIDATE_NUMERICAL_EQUALS(values[0], 3.0);
          VALIDATE_NUMERICAL_EQUALS(values[1], 5.0);
        }
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A repeated time stamp overwrites the newest row in the file") {
  GIVEN("A database that writes into a fresh file") {
    const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_replace");
    const std::string fileName = baseName + ".csv";
    std::filesystem::remove(fileName);
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);

    WHEN("The database adds the values one, two and three at the time stamps one, two and two") {
      const double firstData[1]       = {1.0};
      const double secondData[1]      = {2.0};
      const double replacementData[1] = {3.0};
      database.addGlobalSnapshot(1.0, 1, firstData);
      database.addGlobalSnapshot(2.0, 1, secondData);
      database.addGlobalSnapshot(2.0, 1, replacementData);
      const std::vector<std::string> lines = readNonEmptyLines(fileName);

      THEN("The file contains the header and the values one at the time stamp one and three at the time stamp two") {
        VALIDATE_EQUALS(static_cast<int>(lines.size()), 3, fileName);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[1])[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[1])[1], 1.0);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[2])[0], 2.0);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[2])[1], 3.0);
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A snapshot within the time delta of the newest one does not reach the file") {
  GIVEN("A database with the time delta one that writes into a fresh file") {
    const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_throttle");
    const std::string fileName = baseName + ".csv";
    std::filesystem::remove(fileName);
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);
    database.setTimeDeltaBetweenTwoSnapshots(1.0);

    WHEN("The database adds snapshots at the time stamps 0.5, 0.9 and 1.6") {
      const double data[1] = {0.0};
      database.addGlobalSnapshot(0.5, 1, data);
      database.addGlobalSnapshot(0.9, 1, data);
      database.addGlobalSnapshot(1.6, 1, data);
      const std::vector<std::string> lines = readNonEmptyLines(fileName);

      THEN("The file contains the header and the rows of the time stamps 0.5 and 1.6") {
        VALIDATE_EQUALS(static_cast<int>(lines.size()), 3, fileName);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[1])[0], 0.5);
        VALIDATE_NUMERICAL_EQUALS(parseRow(lines[2])[0], 1.6);
      }
    }
    std::filesystem::remove(fileName);
  }
}

UNIT_TEST("A snapshot is due for an empty database, for the newest time stamp and one time delta after it") {
  GIVEN("A database with the time delta one") {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setTimeDeltaBetweenTwoSnapshots(1.0);

    WHEN("The test asks the empty database for a snapshot at the time stamp 0.5") {
      const bool isDue = database.isSnapshotDue(0.5);

      THEN("The snapshot is due") { VALIDATE_EQUALS(isDue, true); }
    }

    WHEN("The database adds a snapshot at the time stamp 0.5") {
      const double data[1] = {0.0};
      database.addGlobalSnapshot(0.5, 1, data);
      const bool isDueAtTheNewestTimeStamp = database.isSnapshotDue(0.5);
      const bool isDueWithinTheDelta       = database.isSnapshotDue(0.9);
      const bool isDueAfterTheDelta        = database.isSnapshotDue(1.5);

      THEN("A snapshot is due only at the newest time stamp and one time delta later") {
        VALIDATE_EQUALS(isDueAtTheNewestTimeStamp, true);
        VALIDATE_EQUALS(isDueWithinTheDelta, false);
        VALIDATE_EQUALS(isDueAfterTheDelta, true);
      }
    }
  }
}

UNIT_TEST("Under the rule EveryLaterTimestamp a snapshot is due for every later time stamp only") {
  GIVEN("A database with the time delta one and a snapshot at the time stamp 0.5") {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setTimeDeltaBetweenTwoSnapshots(1.0);
    const double data[1] = {0.0};
    database.addGlobalSnapshot(0.5, 1, data);
    const bool isDueWithinTheDelta = database.isSnapshotDue(0.75);

    WHEN("The database switches to the rule EveryLaterTimestamp") {
      database.setSnapshotRule(tarch::database::SnapshotRule::EveryLaterTimestamp);
      const bool isDueAtALaterTimeStamp    = database.isSnapshotDue(0.75);
      const bool isDueAtTheNewestTimeStamp = database.isSnapshotDue(0.5);
      const bool isDueAtAnEarlierTimeStamp = database.isSnapshotDue(0.25);

      THEN("A snapshot is due only at the later time stamp within the time delta") {
        VALIDATE_EQUALS(isDueWithinTheDelta, false);
        VALIDATE_EQUALS(isDueAtALaterTimeStamp, true);
        VALIDATE_EQUALS(isDueAtTheNewestTimeStamp, false);
        VALIDATE_EQUALS(isDueAtAnEarlierTimeStamp, false);
      }

      AND_WHEN("The database adds a snapshot at the time stamp 0.75 and switches back to the rule Deltas") {
        database.addGlobalSnapshot(0.75, 1, data);
        const bool isDueAtTheAddedTimeStamp = database.isSnapshotDue(0.75);
        database.setSnapshotRule(tarch::database::SnapshotRule::Deltas);
        const bool isDueAtTheNewestTimeStampUnderTheDeltas = database.isSnapshotDue(0.75);
        const bool isDueWithinTheNewDelta                  = database.isSnapshotDue(1.5);
        const bool isDueAfterTheNewDelta                   = database.isSnapshotDue(1.75);

        THEN("The added time stamp is due only under the rule Deltas and the time delta counts from it") {
          VALIDATE_EQUALS(isDueAtTheAddedTimeStamp, false);
          VALIDATE_EQUALS(isDueAtTheNewestTimeStampUnderTheDeltas, true);
          VALIDATE_EQUALS(isDueWithinTheNewDelta, false);
          VALIDATE_EQUALS(isDueAfterTheNewDelta, true);
        }
      }
    }
  }
}

UNIT_TEST("A database without an output file name keeps its snapshots in memory only") {
  GIVEN("A database with the time delta one and without an output file name") {
    const std::string fileNameOfAnEmptyBaseName = ".csv";
    std::filesystem::remove(fileNameOfAnEmptyBaseName);
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setTimeDeltaBetweenTwoSnapshots(1.0);

    WHEN("The database adds a snapshot at the time stamp 0.5") {
      const double data[1] = {2.0};
      database.addGlobalSnapshot(0.5, 1, data);
      const bool fileExists          = std::filesystem::exists(fileNameOfAnEmptyBaseName);
      const bool isDueWithinTheDelta = database.isSnapshotDue(0.9);

      THEN("The database writes zero files and isSnapshotDue() returns false within the time delta of the kept one") {
        VALIDATE_EQUALS(fileExists, false);
        VALIDATE_EQUALS(isDueWithinTheDelta, false);
      }
    }
  }
}
