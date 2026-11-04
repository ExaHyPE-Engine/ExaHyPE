// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SnapshotDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Error.h"

tarch::database::SnapshotDatabase::SnapshotDatabase(const double timeDelta) :
  _timeDelta(timeDelta) {}

tarch::database::SnapshotDatabase::Snapshot::Snapshot(double timestamp, int numberOfDataEntries) :
  timestamp(timestamp),
  data(numberOfDataEntries, 0.0) {}

/**
 * Returns the file base name with the suffix .csv.
 */
std::string
tarch::database::SnapshotDatabase::getOutputFileName() const {
  return _fileName + ".csv";
}

/**
 * Writes the snapshots not yet on disk. The first flush truncates the file and writes the header.
 * A failed write marks every snapshot unwritten for the next call.
 */
void
tarch::database::SnapshotDatabase::dumpCSVFile() {
  const std::scoped_lock lock(_mutex);

  if (not hasUnwrittenSnapshots()) {
    return;
  }

  const bool                  writeHeaderNow = not _csvHeaderWritten;
  const std::string           fileName       = getOutputFileName();
  const std::filesystem::path outputPath(fileName);
  if (writeHeaderNow and outputPath.has_parent_path()) {
    std::error_code directoryError;
    std::filesystem::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError) {
      LOG_ERROR(
        "The snapshot database cannot create the directory " << outputPath.parent_path() << " ("
                                                             << directoryError.message() << ")."
      );
      return;
    }
  }

  std::ofstream file(fileName, writeHeaderNow ? std::ios::trunc : std::ios::app);
  if (not file.is_open()) {
    LOG_ERROR("The snapshot database cannot open the file '" << fileName << "'.");
    return;
  }

  prepareOutputStream(file);
  if (writeHeaderNow) {
    writeHeader(file);
  }
  writeUnwrittenSnapshots(file);
  file.flush();

  if (file.good()) {
    _csvHeaderWritten = true;
  } else {
    LOG_ERROR("The snapshot database cannot write the file '" << fileName << "'.");
    resetWriteTracking();
    _csvHeaderWritten = false;
  }
}

/**
 * Erases the whole database. The next flush rewrites the file from scratch.
 * The caller owns the lock when lockMutex is false.
 */
void
tarch::database::SnapshotDatabase::clear(bool lockMutex) {
  std::unique_lock<std::mutex> lock(_mutex, std::defer_lock);

  if (lockMutex) {
    lock.lock();
  }

  clearStorage();
  resetWriteTracking();
  _csvHeaderWritten = false;
}

void
tarch::database::SnapshotDatabase::setOutputFileName(const std::string& filename) {
  const std::scoped_lock lock(_mutex);
  _fileName         = filename;
  _csvHeaderWritten = false;
  resetWriteTracking();
}

void
tarch::database::SnapshotDatabase::setOutputPrecision(int precision) {
  const std::scoped_lock lock(_mutex);
  _precision = precision;
}

void
tarch::database::SnapshotDatabase::setTimeDeltaBetweenTwoSnapshots(double value) {
  ASSERTION(value >= 0.0);
  const std::scoped_lock lock(_mutex);
  _timeDelta = value;
}

/**
 * Selects the snapshots the database stores. Under Deltas the database stores a snapshot once its
 * deltas have passed since the newest one. Under EveryLaterTimestamp it stores every snapshot with
 * a later time stamp than the newest one.
 */
void
tarch::database::SnapshotDatabase::setSnapshotRule(const tarch::database::SnapshotRule rule) {
  const std::scoped_lock lock(_mutex);
  _snapshotRule = rule;
}
