// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SnapshotDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

tarch::database::SnapshotDatabase::SnapshotDatabase(double timeDelta, bool deltasAreRelative) :
  _fileName(""),
  _timeDelta(timeDelta),
  _precision(0),
  _deltasAreRelative(deltasAreRelative),
  _csvHeaderWritten(false),
  _rank(-1) {}

tarch::database::SnapshotDatabase::Snapshot::Snapshot(double timestamp_, int numberOfDataEntries) :
  timestamp(timestamp_),
  data(numberOfDataEntries, 0.0) {}

/**
 * The name of the CSV file. Only the file base name has to be set by the user;
 * the suffix is added here. Global data goes into one file, so this default
 * appends nothing but the extension.
 */
std::string tarch::database::SnapshotDatabase::outputFileName() const { return _fileName + ".csv"; }

/**
 * Append the snapshots not yet on disk to the CSV file. The header is written,
 * and the file truncated, on the first flush; every later flush appends. So a
 * long run grows the file in place instead of rewriting it over and over.
 */
void tarch::database::SnapshotDatabase::dumpCSVFile() {
  tarch::multicore::Lock lock(_semaphore);

  if (not hasUnwrittenSnapshots()) {
    return;
  }

  const bool                  writeHeaderNow = not _csvHeaderWritten;
  const std::string           fileName       = outputFileName();
  const std::filesystem::path outputPath(fileName);
  if (writeHeaderNow and outputPath.has_parent_path()) {
    std::filesystem::create_directories(outputPath.parent_path());
  }

  std::ofstream file(fileName, writeHeaderNow ? std::ios::trunc : std::ios::app);
  prepareOutputStream(file);
  if (writeHeaderNow) {
    writeHeader(file);
    _csvHeaderWritten = true;
  }
  writeUnwrittenSnapshots(file);
}

/**
 * Erase the whole database. As nothing is on disk anymore, the next flush
 * rewrites the file from scratch. Thread-safe if lockSemaphore is set.
 */
void tarch::database::SnapshotDatabase::clear(bool lockSemaphore) {
  tarch::multicore::Lock lock(_semaphore, false);

  if (lockSemaphore) {
    lock.lock();
  }

  clearStorage();
  resetWriteTracking();
  _csvHeaderWritten = false;
}

void tarch::database::SnapshotDatabase::setOutputFileName(const std::string& filename) {
  _fileName         = filename;
  _csvHeaderWritten = false;
  resetWriteTracking();
}

void tarch::database::SnapshotDatabase::setOutputPrecision(int precision) { _precision = precision; }

void tarch::database::SnapshotDatabase::setTimeDeltaBetweenTwoSnapshots(double value) {
  assertion(value >= 0.0);
  _timeDelta = value;
}
