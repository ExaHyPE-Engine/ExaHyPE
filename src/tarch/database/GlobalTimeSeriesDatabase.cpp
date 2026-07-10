// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GlobalTimeSeriesDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/multicore/Lock.h"

#include <algorithm>
#include <vector>

tarch::database::GlobalTimeSeriesDatabase::GlobalTimeSeriesDatabase(double timeDelta, bool deltasAreRelative) :
  SnapshotDatabase(timeDelta, deltasAreRelative),
  _dataName(" data "),
  _numberOfGlobalDataPoints(0),
  _numberOfEntriesWrittenToFile(0) {}

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
  const int total    = static_cast<int>(_data.size());
  const int newCount = total - _numberOfEntriesWrittenToFile;
  // New rows sit at the front of the newest-first list; emit them oldest-first.
  std::vector<const Snapshot*> fresh;
  fresh.reserve(newCount);
  auto it = _data.begin();
  for (int i = 0; i < newCount; i++) {
    fresh.push_back(&(*it));
    ++it;
  }
  for (auto r = fresh.rbegin(); r != fresh.rend(); ++r) {
    writeRow(file, **r);
  }
  _numberOfEntriesWrittenToFile = total;
}

void tarch::database::GlobalTimeSeriesDatabase::clearStorage() { _data.clear(); }

void tarch::database::GlobalTimeSeriesDatabase::resetWriteTracking() { _numberOfEntriesWrittenToFile = 0; }

void tarch::database::GlobalTimeSeriesDatabase::setDataName(const std::string& dataname) { _dataName = dataname; }

/**
 * Determine whether a new measurement is appended as a new time stamp, replaces
 * the newest entry (same time stamp), or is discarded because not enough time
 * has passed.
 */
tarch::database::SnapshotDatabase::AddSnapshotAction
tarch::database::GlobalTimeSeriesDatabase::getAction(double timestamp) {
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
 * Add a measurement of numberOfDataEntries data values for the given time
 * stamp. The number of values has to stay consistent across calls.
 */
void tarch::database::GlobalTimeSeriesDatabase::addGlobalSnapshot(
  double        timestamp,
  int           numberOfDataEntries,
  const double* data
) {
  assertion(_numberOfGlobalDataPoints == numberOfDataEntries or _data.empty());
  _numberOfGlobalDataPoints = std::max(_numberOfGlobalDataPoints, numberOfDataEntries);

  tarch::multicore::Lock lock(_semaphore);
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
  lock.free();

  dumpCSVFile();
}
