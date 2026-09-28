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
 * Returns the name of the CSV file. The caller sets the file base name; the
 * suffix is added here.
 */
std::string
tarch::database::SnapshotDatabase::getOutputFileName() const {
  return _fileName + ".csv";
}

/**
 * Appends snapshots not yet on disk. The first flush truncates the file and
 * writes its header; later flushes append. Directory, open, or write failures
 * leave all snapshots marked as unwritten, so the next call retries by
 * rewriting the complete file.
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
      logError("Could not create output directory " << outputPath.parent_path() << ": " << directoryError.message());
      return;
    }
  }

  std::ofstream file(fileName, writeHeaderNow ? std::ios::trunc : std::ios::app);
  if (not file.is_open()) {
    logError("Could not open snapshot database file " << fileName << ".");
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
    logError("Could not write snapshot database file " << fileName << ".");
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
  assertion(value >= 0.0);
  const std::scoped_lock lock(_mutex);
  _timeDelta = value;
}
