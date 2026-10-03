// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Rank.h"

exahype2::particles::TrajectoryDatabase::Entry::Entry(
  const tarch::la::Vector<DIMENSIONS, double>& position,
  double                                       timestamp,
  int                                          numberOfDataEntries
) :
  Snapshot(timestamp, numberOfDataEntries),
  x(position) {}

exahype2::particles::TrajectoryDatabase::TrajectoryDatabase(
  const double positionDelta,
  const double timeDelta,
  const bool   deltasAreRelative
) :
  SnapshotDatabase(timeDelta),
  _positionDelta(positionDelta),
  _deltasAreRelative(deltasAreRelative) {
  _precision = 8;
}

exahype2::particles::TrajectoryDatabase::~TrajectoryDatabase() { clear(); }

/**
 * Every rank writes its own file, so the file name contains the rank number when more than one
 * rank runs.
 */
std::string
exahype2::particles::TrajectoryDatabase::getOutputFileName() const {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    return _fileName + "-rank-" + std::to_string(_rank) + ".csv";
  }
  return _fileName + ".csv";
}

void
exahype2::particles::TrajectoryDatabase::writeRow(
  std::ostream&              file,
  const std::pair<int, int>& number,
  const Entry&               snapshot
) const {
  file << number.first << ", " << number.second << ", " << snapshot.timestamp;
#if DIMENSIONS == 2
  file << ", " << snapshot.x(0) << ", " << snapshot.x(1);
#else
  file << ", " << snapshot.x(0) << ", " << snapshot.x(1) << ", " << snapshot.x(2);
#endif
  for (const double value : snapshot.data) {
    file << ", " << value;
  }
  file << std::endl;
}

void
exahype2::particles::TrajectoryDatabase::prepareOutputStream(std::ostream& file) const {
  file << std::scientific << std::setprecision(_precision);
}

void
exahype2::particles::TrajectoryDatabase::writeHeader(std::ostream& file) const {
#if DIMENSIONS == 2
  file << "number(0), number(1), t, x(0), x(1)";
#else
  file << "number(0), number(1), t, x(0), x(1), x(2)";
#endif
  for (int dataIndex = 0; dataIndex < _numberOfDataPointsPerParticle; dataIndex++) {
    file << ", data(" << dataIndex << ")";
  }
  file << std::endl;
}

bool
exahype2::particles::TrajectoryDatabase::hasUnwrittenSnapshots() const {
  for (const auto& particleEntry : _data) {
    const auto snapshotCountEntry = _writtenSnapshotsPerParticle.find(particleEntry.first);
    const int  writtenSnapshots =
      (snapshotCountEntry == _writtenSnapshotsPerParticle.end()) ? 0 : snapshotCountEntry->second;
    if (static_cast<int>(particleEntry.second.size()) > writtenSnapshots) {
      return true;
    }
  }
  return false;
}

void
exahype2::particles::TrajectoryDatabase::writeUnwrittenSnapshots(std::ostream& file) {
  for (const auto& particleEntry : _data) {
    const int totalSnapshots       = static_cast<int>(particleEntry.second.size());
    int&      writtenSnapshots     = _writtenSnapshotsPerParticle[particleEntry.first];
    const int numberOfNewSnapshots = totalSnapshots - writtenSnapshots;
    if (numberOfNewSnapshots > 0) {
      // The list stores the newest snapshot first, so the loop writes the new snapshots in reverse.
      std::vector<const Entry*> unwrittenSnapshots;
      unwrittenSnapshots.reserve(numberOfNewSnapshots);
      auto snapshotIterator = particleEntry.second.begin();
      for (int snapshotIndex = 0; snapshotIndex < numberOfNewSnapshots; snapshotIndex++) {
        unwrittenSnapshots.push_back(&(*snapshotIterator));
        ++snapshotIterator;
      }
      for (auto snapshot = unwrittenSnapshots.rbegin(); snapshot != unwrittenSnapshots.rend(); ++snapshot) {
        writeRow(file, particleEntry.first, **snapshot);
      }
      writtenSnapshots = totalSnapshots;
    }
  }
}

void
exahype2::particles::TrajectoryDatabase::clearStorage() {
  _data.clear();
}

void
exahype2::particles::TrajectoryDatabase::resetWriteTracking() {
  _writtenSnapshotsPerParticle.clear();
}

/**
 * Returns the action for a snapshot of particle number at x and timestamp: Append, Replace or
 * Ignore. The caller holds _mutex.
 */
exahype2::particles::TrajectoryDatabase::AddSnapshotAction
exahype2::particles::TrajectoryDatabase::getAction(
  const std::pair<int, int>&                   number,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const double                                 timestamp
) const {
  const auto particle = _data.find(number);
  if (particle == _data.end() or particle->second.empty()) {
    return AddSnapshotAction::Append;
  }

  const auto&  newest           = particle->second.front();
  const double positionDelta    = tarch::la::norm2(newest.x - x);
  const double maxPositionDelta = std::max(positionDelta, _maxPositionDelta);
  if (tarch::la::equals(newest.timestamp, timestamp)) {
    return tarch::la::equals(newest.x, x) ? AddSnapshotAction::Ignore : AddSnapshotAction::Replace;
  }
  if (
    _deltasAreRelative and maxPositionDelta >= DeltaCutoffThreshold
    and positionDelta / maxPositionDelta >= _positionDelta
  ) {
    return AddSnapshotAction::Append;
  }
  if (not _deltasAreRelative and positionDelta >= _positionDelta) {
    return AddSnapshotAction::Append;
  }
  if (timestamp - newest.timestamp >= _timeDelta and _timeDelta > 0.0) {
    return AddSnapshotAction::Append;
  }
  return AddSnapshotAction::Ignore;
}

/**
 * Returns the action of getAction(), raises _maxPositionDelta to the position delta of the particle
 * and inserts an empty list for a new particle. The caller holds _mutex.
 */
exahype2::particles::TrajectoryDatabase::AddSnapshotAction
exahype2::particles::TrajectoryDatabase::prepareSnapshot(
  const std::pair<int, int>&                   number,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const double                                 timestamp
) {
  const AddSnapshotAction action    = getAction(number, x, timestamp);
  const auto&             snapshots = _data[number];
  if (not snapshots.empty()) {
    _maxPositionDelta = std::max(tarch::la::norm2(snapshots.front().x - x), _maxPositionDelta);
  }
  if (action == AddSnapshotAction::Replace) {
    logWarning(
      "Particle " << number.first << "x" << number.second << " has two locations " << x << " and "
                  << snapshots.front().x << " for same time stamp " << timestamp << ". This is inconsistent."
    );
  }
  return action;
}

/**
 * Returns whether addParticleSnapshot() stores the snapshot of the particle number at position x and timestamp.
 */
bool
exahype2::particles::TrajectoryDatabase::isSnapshotDue(
  const std::pair<int, int>&                   number,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const double                                 timestamp
) {
  const std::scoped_lock lock(_mutex);
  return getAction(number, x, timestamp) != AddSnapshotAction::Ignore;
}

void
exahype2::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x
) {
  {
    const std::scoped_lock lock(_mutex);
    if (_rank < 0) {
      _rank = tarch::mpi::Rank::getInstance().getRank();
    }
    switch (prepareSnapshot(number, x, timestamp)) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.at(number).push_front(Entry(x, timestamp, _numberOfDataPointsPerParticle));
      break;
    case AddSnapshotAction::Replace:
      _data.at(number).front().x = x;
      break;
    }
  }

  dumpCSVFile();
}

void
exahype2::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x
) {
  addParticleSnapshot(std::pair<int, int>(number0, number1), timestamp, x);
}

void
exahype2::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  int                                          numberOfDataEntries,
  const double*                                data
) {
  {
    const std::scoped_lock lock(_mutex);
    if (not _data.empty() and _numberOfDataPointsPerParticle != numberOfDataEntries) {
      std::ostringstream message;
      message << "Inconsistent number of particle data entries: database expects " << _numberOfDataPointsPerParticle
              << " but received " << numberOfDataEntries << ".";
      tarch::terminateWithError(message.str());
    }
    assertion(_numberOfDataPointsPerParticle == numberOfDataEntries or _data.empty());
    _numberOfDataPointsPerParticle = std::max(_numberOfDataPointsPerParticle, numberOfDataEntries);

    if (_rank < 0) {
      _rank = tarch::mpi::Rank::getInstance().getRank();
    }
    switch (prepareSnapshot(number, x, timestamp)) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.at(number).push_front(Entry(x, timestamp, _numberOfDataPointsPerParticle));
      std::copy_n(data, _numberOfDataPointsPerParticle, _data.at(number).front().data.begin());
      break;
    case AddSnapshotAction::Replace:
      _data.at(number).front().x = x;
      std::copy_n(data, _numberOfDataPointsPerParticle, _data.at(number).front().data.begin());
      break;
    }
  }

  dumpCSVFile();
}

void
exahype2::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  int                                          numberOfDataEntries,
  const double*                                data
) {
  addParticleSnapshot(std::pair<int, int>(number0, number1), timestamp, x, numberOfDataEntries, data);
}
