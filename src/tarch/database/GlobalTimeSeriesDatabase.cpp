// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GlobalTimeSeriesDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"

tarch::database::GlobalTimeSeriesDatabase::GlobalTimeSeriesDatabase(double timeDelta, bool deltasAreRelative) :
  SnapshotDatabase(timeDelta, deltasAreRelative) {}

tarch::database::GlobalTimeSeriesDatabase::~GlobalTimeSeriesDatabase() { clear(); }

void tarch::database::GlobalTimeSeriesDatabase::writeRow(std::ostream& file, const Snapshot& snapshot) const {
  file << snapshot.timestamp;
  for (const double value : snapshot.data) {
    file << ", " << value;
  }
  file << std::endl;
}

void tarch::database::GlobalTimeSeriesDatabase::prepareOutputStream(std::ostream& file) const {
  file << std::scientific;
  if (_precision > 0) {
    file.precision(_precision);
  }
}

void tarch::database::GlobalTimeSeriesDatabase::writeHeader(std::ostream& file) const {
  file << "t, " << _dataName << std::endl;
}

bool tarch::database::GlobalTimeSeriesDatabase::hasUnwrittenSnapshots() const {
  return static_cast<int>(_data.size()) > _numberOfEntriesWrittenToFile;
}

void tarch::database::GlobalTimeSeriesDatabase::writeUnwrittenSnapshots(std::ostream& file) {
  const int totalSnapshotCount     = static_cast<int>(_data.size());
  const int unwrittenSnapshotCount = totalSnapshotCount - _numberOfEntriesWrittenToFile;
  // New rows sit at the front of the newest-first list; emit them oldest-first.
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

void tarch::database::GlobalTimeSeriesDatabase::clearStorage() { _data.clear(); }

void tarch::database::GlobalTimeSeriesDatabase::resetWriteTracking() { _numberOfEntriesWrittenToFile = 0; }

void tarch::database::GlobalTimeSeriesDatabase::setDataName(const std::string& dataName) { _dataName = dataName; }

/**
 * Determines whether a new measurement is appended as a new time stamp, replaces
 * the newest entry (same time stamp), or is discarded because not enough time
 * has passed.
 */
tarch::database::SnapshotDatabase::AddSnapshotAction tarch::database::GlobalTimeSeriesDatabase::getAction(
  double timestamp
) {
  if (_data.empty()) {
    return AddSnapshotAction::Append;
  } else if (tarch::la::equals(_data.front().timestamp, timestamp)) {
    return AddSnapshotAction::Replace;
  } else if (timestamp - _data.front().timestamp >= _timeDelta and _timeDelta >= 0.0) {
    return AddSnapshotAction::Append;
  } else {
    return AddSnapshotAction::Ignore;
  }
}

/**
 * Adds one measurement while holding the database lock across the decision and
 * mutation. A repeated time stamp replaces the newest row; a later accepted
 * time stamp is prepended. The number of values remains constant across calls.
 */
void tarch::database::GlobalTimeSeriesDatabase::addGlobalSnapshot(
  double        timestamp,
  int           numberOfDataEntries,
  const double* data
) {
  {
    const std::scoped_lock lock(_mutex);
    assertion(_numberOfGlobalDataPoints == numberOfDataEntries or _data.empty());
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
      break;
    }
  }

  dumpCSVFile();
}
