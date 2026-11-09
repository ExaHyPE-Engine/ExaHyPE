// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GlobalTimeSeriesDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"

tarch::database::GlobalTimeSeriesDatabase::GlobalTimeSeriesDatabase(const double timeDelta) :
  SnapshotDatabase(timeDelta) {}

tarch::database::GlobalTimeSeriesDatabase::~GlobalTimeSeriesDatabase() { clear(); }

void
tarch::database::GlobalTimeSeriesDatabase::writeRow(std::ostream& file, const Snapshot& snapshot) const {
  file << snapshot.timestamp;
  for (const double value : snapshot.data) {
    file << ", " << value;
  }
  file << std::endl;
}

void
tarch::database::GlobalTimeSeriesDatabase::prepareOutputStream(std::ostream& file) const {
  file << std::scientific;
  if (_precision > 0) {
    file.precision(_precision);
  }
}

void
tarch::database::GlobalTimeSeriesDatabase::writeHeader(std::ostream& file) const {
  file << "t, " << _dataName << std::endl;
}

bool
tarch::database::GlobalTimeSeriesDatabase::hasUnwrittenSnapshots() const {
  return static_cast<int>(_data.size()) > _numberOfEntriesWrittenToFile;
}

void
tarch::database::GlobalTimeSeriesDatabase::writeUnwrittenSnapshots(std::ostream& file) {
  const int totalSnapshotCount     = static_cast<int>(_data.size());
  const int unwrittenSnapshotCount = totalSnapshotCount - _numberOfEntriesWrittenToFile;
  // The list stores the newest row first. The flush writes the unwritten rows in reverse.
  std::vector<const Snapshot*> unwrittenSnapshots;
  unwrittenSnapshots.reserve(unwrittenSnapshotCount);
  auto snapshot = _data.begin();
  for (int collectedSnapshotCount = 0; collectedSnapshotCount < unwrittenSnapshotCount; collectedSnapshotCount++) {
    unwrittenSnapshots.push_back(&(*snapshot));
    ++snapshot;
  }
  for (auto snapshotToWrite = unwrittenSnapshots.rbegin(); snapshotToWrite != unwrittenSnapshots.rend();
       ++snapshotToWrite) {
    writeRow(file, **snapshotToWrite);
  }
  _numberOfEntriesWrittenToFile = totalSnapshotCount;
}

void
tarch::database::GlobalTimeSeriesDatabase::clearStorage() {
  _data.clear();
}

void
tarch::database::GlobalTimeSeriesDatabase::resetWriteTracking() {
  _numberOfEntriesWrittenToFile = 0;
}

void
tarch::database::GlobalTimeSeriesDatabase::setDataName(const std::string& dataName) {
  _dataName = dataName;
}

/**
 * Returns whether a measurement at timestamp appends a row, replaces the newest row with the same
 * time stamp, or comes too early and is ignored. Under SnapshotRule::EveryLaterTimestamp only a
 * later time stamp than the newest row appends a row.
 */
tarch::database::SnapshotDatabase::AddSnapshotAction
tarch::database::GlobalTimeSeriesDatabase::getAction(const double timestamp) const {
  if (_data.empty()) {
    return AddSnapshotAction::Append;
  } else if (_snapshotRule == tarch::database::SnapshotRule::EveryLaterTimestamp) {
    return tarch::la::greater(timestamp, _data.front().timestamp) ? AddSnapshotAction::Append
                                                                  : AddSnapshotAction::Ignore;
  } else if (tarch::la::equals(_data.front().timestamp, timestamp)) {
    return AddSnapshotAction::Replace;
  } else if (timestamp - _data.front().timestamp >= _timeDelta and _timeDelta >= 0.0) {
    return AddSnapshotAction::Append;
  } else {
    return AddSnapshotAction::Ignore;
  }
}

/**
 * Returns whether addGlobalSnapshot() appends or replaces a snapshot at timestamp.
 */
bool
tarch::database::GlobalTimeSeriesDatabase::isSnapshotDue(const double timestamp) {
  const std::scoped_lock lock(_mutex);
  return getAction(timestamp) != AddSnapshotAction::Ignore;
}

/**
 * Adds one measurement. A repeated time stamp replaces the newest row. A database without an
 * output file name keeps its rows in memory only.
 */
void
tarch::database::GlobalTimeSeriesDatabase::addGlobalSnapshot(
  const double        timestamp,
  const int           numberOfDataEntries,
  const double* const data
) {
  ASSERTION(data != nullptr);
  ASSERTION(numberOfDataEntries > 0, numberOfDataEntries);
  {
    const std::scoped_lock lock(_mutex);
    ASSERTION(_numberOfGlobalDataPoints == numberOfDataEntries or _data.empty());
    _numberOfGlobalDataPoints = std::max(_numberOfGlobalDataPoints, numberOfDataEntries);
    switch (getAction(timestamp)) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.push_front(Snapshot(timestamp, _numberOfGlobalDataPoints));
      std::copy_n(data, _numberOfGlobalDataPoints, _data.front().data.begin());
      break;
    case AddSnapshotAction::Replace:
      std::copy_n(data, _numberOfGlobalDataPoints, _data.front().data.begin());
      if (not hasUnwrittenSnapshots()) {
        resetWriteTracking();
        _csvHeaderWritten = false;
      }
      break;
    }
  }

  if (not _fileName.empty()) {
    dumpCSVFile();
  }
}
