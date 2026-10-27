// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GlobalTimeSeriesDatabaseTest.h"

#include "tarch/database/GlobalTimeSeriesDatabase.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::string getTestFileBaseName(const std::string& name) {
    return (tarch::tests::getWritableTestDirectory() / name).string();
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

tarch::database::tests::GlobalTimeSeriesDatabaseTest::GlobalTimeSeriesDatabaseTest() :
  tarch::tests::TestCase("tarch::database::tests::GlobalTimeSeriesDatabaseTest") {}

void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::run() {
  // Only the global master writes the global database. No two ranks write the same file.
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }
  testMethod(testAppendsChronologicallyWithoutDuplication);
  testMethod(testClearRewritesFileFromScratch);
  testMethod(testARepeatedTimeStampOverwritesTheRowInTheFile);
  testMethod(testTimeDeltaThrottlesSnapshots);
  testMethod(testASnapshotIsDueForTheNewestTimeStampAndAfterTheTimeDelta);
  testMethod(testEveryLaterTimestampIsDueBeforeTheTimeDelta);
  testMethod(testADatabaseWithoutFileNameWritesNoFile);
}

void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testAppendsChronologicallyWithoutDuplication() {
  const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_append");
  const std::string fileName = baseName + ".csv";
  std::filesystem::remove(fileName);

  {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);
    for (int step = 1; step <= 4; step++) {
      const double data[2] = {10.0 * step, 20.0 * step};
      database.addGlobalSnapshot(static_cast<double>(step), 2, data);
    }
  }

  const std::vector<std::string> lines = readNonEmptyLines(fileName);
  validateEqualsWithParams1(static_cast<int>(lines.size()), 5, fileName);
  if (lines.size() == 5) {
    for (int row = 0; row < 4; row++) {
      const std::vector<double> values = parseRow(lines[row + 1]);
      validateEquals(static_cast<int>(values.size()), 3);
      validateNumericalEquals(values[0], static_cast<double>(row + 1));
      validateNumericalEquals(values[1], 10.0 * (row + 1));
      validateNumericalEquals(values[2], 20.0 * (row + 1));
    }
  }
  std::filesystem::remove(fileName);
}

/**
 * A cleared database rewrites its file with the snapshots added after the clear only.
 */
void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testClearRewritesFileFromScratch() {
  const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_clear");
  const std::string fileName = baseName + ".csv";
  std::filesystem::remove(fileName);

  {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);

    const double firstData[1] = {1.0};
    database.addGlobalSnapshot(1.0, 1, firstData);
    database.addGlobalSnapshot(2.0, 1, firstData);
    validateEqualsWithParams1(static_cast<int>(readNonEmptyLines(fileName).size()), 3, fileName);

    database.clear();

    const double secondData[1] = {5.0};
    database.addGlobalSnapshot(3.0, 1, secondData);

    const std::vector<std::string> lines = readNonEmptyLines(fileName);
    validateEqualsWithParams1(static_cast<int>(lines.size()), 2, fileName);
    if (lines.size() == 2) {
      const std::vector<double> values = parseRow(lines[1]);
      validateEquals(static_cast<int>(values.size()), 2);
      validateNumericalEquals(values[0], 3.0);
      validateNumericalEquals(values[1], 5.0);
    }
  }
  std::filesystem::remove(fileName);
}

void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testARepeatedTimeStampOverwritesTheRowInTheFile() {
  const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_replace");
  const std::string fileName = baseName + ".csv";
  std::filesystem::remove(fileName);

  {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);

    const double firstData[1]       = {1.0};
    const double secondData[1]      = {2.0};
    const double replacementData[1] = {3.0};
    database.addGlobalSnapshot(1.0, 1, firstData);
    database.addGlobalSnapshot(2.0, 1, secondData);
    database.addGlobalSnapshot(2.0, 1, replacementData);

    const std::vector<std::string> lines = readNonEmptyLines(fileName);
    validateEqualsWithParams1(static_cast<int>(lines.size()), 3, fileName);
    if (lines.size() == 3) {
      validateNumericalEquals(parseRow(lines[1])[0], 1.0);
      validateNumericalEquals(parseRow(lines[1])[1], 1.0);
      validateNumericalEquals(parseRow(lines[2])[0], 2.0);
      validateNumericalEquals(parseRow(lines[2])[1], 3.0);
    }
  }
  std::filesystem::remove(fileName);
}

/**
 * A snapshot within the time delta of the newest one does not reach the file.
 */
void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testTimeDeltaThrottlesSnapshots() {
  const std::string baseName = getTestFileBaseName("GlobalTimeSeriesDatabaseTest_throttle");
  const std::string fileName = baseName + ".csv";
  std::filesystem::remove(fileName);

  {
    tarch::database::GlobalTimeSeriesDatabase database;
    database.setOutputFileName(baseName);
    database.setTimeDeltaBetweenTwoSnapshots(1.0);

    const double data[1] = {0.0};
    database.addGlobalSnapshot(0.5, 1, data);
    database.addGlobalSnapshot(0.9, 1, data);
    database.addGlobalSnapshot(1.6, 1, data);

    const std::vector<std::string> lines = readNonEmptyLines(fileName);
    validateEqualsWithParams1(static_cast<int>(lines.size()), 3, fileName);
    if (lines.size() == 3) {
      validateNumericalEquals(parseRow(lines[1])[0], 0.5);
      validateNumericalEquals(parseRow(lines[2])[0], 1.6);
    }
  }
  std::filesystem::remove(fileName);
}

/**
 * A snapshot is due for an empty database, for the newest time stamp and one time delta after it.
 */
void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testASnapshotIsDueForTheNewestTimeStampAndAfterTheTimeDelta() {
  tarch::database::GlobalTimeSeriesDatabase database;
  database.setTimeDeltaBetweenTwoSnapshots(1.0);
  validateEquals(database.isSnapshotDue(0.5), true);

  const double data[1] = {0.0};
  database.addGlobalSnapshot(0.5, 1, data);
  validateEquals(database.isSnapshotDue(0.5), true);
  validateEquals(database.isSnapshotDue(0.9), false);
  validateEquals(database.isSnapshotDue(1.5), true);
}

/**
 * Under SnapshotRule::EveryLaterTimestamp a snapshot is due for every later time stamp and for no other one.
 */
void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testEveryLaterTimestampIsDueBeforeTheTimeDelta() {
  tarch::database::GlobalTimeSeriesDatabase database;
  database.setTimeDeltaBetweenTwoSnapshots(1.0);
  const double data[1] = {0.0};
  database.addGlobalSnapshot(0.5, 1, data);
  validateEquals(database.isSnapshotDue(0.75), false);

  database.setSnapshotRule(tarch::database::SnapshotRule::EveryLaterTimestamp);
  validateEquals(database.isSnapshotDue(0.75), true);
  validateEquals(database.isSnapshotDue(0.5), false);
  validateEquals(database.isSnapshotDue(0.25), false);
  database.addGlobalSnapshot(0.75, 1, data);
  validateEquals(database.isSnapshotDue(0.75), false);

  database.setSnapshotRule(tarch::database::SnapshotRule::Deltas);
  validateEquals(database.isSnapshotDue(0.75), true);
  validateEquals(database.isSnapshotDue(1.5), false);
  validateEquals(database.isSnapshotDue(1.75), true);
}

/**
 * A database without an output file name keeps its snapshots and creates no file.
 */
void
tarch::database::tests::GlobalTimeSeriesDatabaseTest::testADatabaseWithoutFileNameWritesNoFile() {
  const std::string fileNameOfAnEmptyBaseName = ".csv";
  std::filesystem::remove(fileNameOfAnEmptyBaseName);

  tarch::database::GlobalTimeSeriesDatabase database;
  database.setTimeDeltaBetweenTwoSnapshots(1.0);
  const double data[1] = {2.0};
  database.addGlobalSnapshot(0.5, 1, data);

  validateEquals(std::filesystem::exists(fileNameOfAnEmptyBaseName), false);
  validateEquals(database.isSnapshotDue(0.9), false);
}
