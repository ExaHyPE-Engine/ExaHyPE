// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryDatabase.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

toolbox::particles::TrajectoryDatabase::Entry::Entry(
  const tarch::la::Vector<DIMENSIONS, double>& x_,
  double                                       timestamp_,
  int                                          numberOfDataEntries
) :
  Snapshot(timestamp_, numberOfDataEntries),
  x(x_) {}

toolbox::particles::TrajectoryDatabase::TrajectoryDatabase(
  double positionDelta,
  double timeDelta,
  bool   deltasAreRelative
) :
  SnapshotDatabase(timeDelta, deltasAreRelative),
  _positionDelta(positionDelta),
  _maxPositionDelta(0.0),
  _numberOfDataPointsPerParticle(0) {
  _precision = 8;
}

toolbox::particles::TrajectoryDatabase::~TrajectoryDatabase() { clear(); }

/**
 * Particles are spread over the ranks, so each rank writes its own file to
 * avoid clashes. In a serial run the rank suffix is dropped.
 */
std::string toolbox::particles::TrajectoryDatabase::outputFileName() const {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 0) {
    return _fileName + "-rank-" + std::to_string(_rank) + ".csv";
  }
  return _fileName + ".csv";
}

void toolbox::particles::TrajectoryDatabase::writeRow(
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

void toolbox::particles::TrajectoryDatabase::prepareOutputStream(std::ostream& file) const {
  file << std::scientific << std::setprecision(_precision);
}

void toolbox::particles::TrajectoryDatabase::writeHeader(std::ostream& file) const {
#if DIMENSIONS == 2
  file << "number(0), number(1), t, x(0), x(1)";
#else
  file << "number(0), number(1), t, x(0), x(1), x(2)";
#endif
  for (int i = 0; i < _numberOfDataPointsPerParticle; i++) {
    file << ", data(" << i << ")";
  }
  file << std::endl;
}

bool toolbox::particles::TrajectoryDatabase::hasUnwrittenSnapshots() const {
  for (const auto& particle : _data) {
    auto      it      = _writtenSnapshotsPerParticle.find(particle.first);
    const int written = (it == _writtenSnapshotsPerParticle.end()) ? 0 : it->second;
    if (static_cast<int>(particle.second.size()) > written) {
      return true;
    }
  }
  return false;
}

void toolbox::particles::TrajectoryDatabase::writeUnwrittenSnapshots(std::ostream& file) {
  for (const auto& particle : _data) {
    const int total    = static_cast<int>(particle.second.size());
    int&      written  = _writtenSnapshotsPerParticle[particle.first];
    const int newCount = total - written;
    if (newCount > 0) {
      // New snapshots sit at the front of the newest-first list; emit them oldest-first.
      std::vector<const Entry*> fresh;
      fresh.reserve(newCount);
      auto it = particle.second.begin();
      for (int i = 0; i < newCount; i++) {
        fresh.push_back(&(*it));
        ++it;
      }
      for (auto r = fresh.rbegin(); r != fresh.rend(); ++r) {
        writeRow(file, particle.first, **r);
      }
      written = total;
    }
  }
}

void toolbox::particles::TrajectoryDatabase::clearStorage() { _data.clear(); }

void toolbox::particles::TrajectoryDatabase::resetWriteTracking() { _writtenSnapshotsPerParticle.clear(); }

void toolbox::particles::TrajectoryDatabase::setPositionDeltaBetweenTwoSnapshots(double value, bool deltasAreRelative) {
  assertion(value >= 0.0);
  _positionDelta     = value;
  _maxPositionDelta  = 1e-20;
  _deltasAreRelative = deltasAreRelative;
}

/**
 * A new snapshot is kept if the particle is new, if it has moved far enough, or
 * if enough time has passed. Two positions for the same time stamp are flagged
 * as inconsistent and replace the existing one.
 */
toolbox::particles::TrajectoryDatabase::AddSnapshotAction toolbox::particles::TrajectoryDatabase::getAction(
  const std::pair<int, int>&                   number,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  double                                       timestamp
) {
  auto [iterator, inserted] = _data.try_emplace(number);
  const auto& entries       = iterator->second;
  if (inserted or entries.empty()) {
    return AddSnapshotAction::Append;
  }

  const auto&  newest = entries.front();
  const auto   oldX   = newest.x;
  const double delta  = tarch::la::norm2(oldX - x);
  _maxPositionDelta   = std::max(delta, _maxPositionDelta);

  if (tarch::la::equals(newest.timestamp, timestamp) and not tarch::la::equals(oldX, x)) {
    logWarning(
      "Particle " << number.first << "x" << number.second << " has two locations " << x << " and " << oldX
                  << " for same time stamp " << timestamp << ". This is inconsistent"
    );
    return AddSnapshotAction::Replace;
  }
  if (tarch::la::equals(newest.timestamp, timestamp)) {
    return AddSnapshotAction::Ignore;
  }
  if (_deltasAreRelative and _maxPositionDelta >= DeltaCutOffThreshold
      and delta / _maxPositionDelta >= _positionDelta) {
    return AddSnapshotAction::Append;
  }
  if (not _deltasAreRelative and delta >= _positionDelta) {
    return AddSnapshotAction::Append;
  }
  if (timestamp - newest.timestamp >= _timeDelta and _timeDelta > 0.0) {
    return AddSnapshotAction::Append;
  }
  return AddSnapshotAction::Ignore;
}

void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x
) {
  if (_rank < 0) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::multicore::Lock lock(_semaphore);
  switch (getAction(number, x, timestamp)) {
  case AddSnapshotAction::Ignore:
    break;
  case AddSnapshotAction::Append:
    _data.at(number).push_front(Entry(x, timestamp, _numberOfDataPointsPerParticle));
    break;
  case AddSnapshotAction::Replace:
    _data.at(number).front().x = x;
    break;
  }
  lock.free();

  dumpCSVFile();
}

void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x
) {
  addParticleSnapshot(std::pair<int, int>(number0, number1), timestamp, x);
}

void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  int                                          numberOfDataEntries,
  const double*                                data
) {
  if (not _data.empty() and _numberOfDataPointsPerParticle != numberOfDataEntries) {
    std::ostringstream message;
    message << "Inconsistent number of particle data entries: database expects " << _numberOfDataPointsPerParticle
            << " but received " << numberOfDataEntries << ".";
    throw std::runtime_error(message.str());
  }
  assertion(_numberOfDataPointsPerParticle == numberOfDataEntries or _data.empty());
  _numberOfDataPointsPerParticle = std::max(_numberOfDataPointsPerParticle, numberOfDataEntries);

  if (_rank < 0) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::multicore::Lock lock(_semaphore);
  switch (getAction(number, x, timestamp)) {
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
  lock.free();

  dumpCSVFile();
}

void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  int                                          numberOfDataEntries,
  const double*                                data
) {
  addParticleSnapshot(std::pair<int, int>(number0, number1), timestamp, x, numberOfDataEntries, data);
}
