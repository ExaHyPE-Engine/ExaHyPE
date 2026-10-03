// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  class TrajectoryDatabase;
} // namespace exahype2::particles

/**
 * Series of position snapshots with optional data per particle, appended to a CSV file. A pair of
 * integers identifies a particle. The threads of a traversal add snapshots concurrently, so
 * addParticleSnapshot() locks _mutex.
 */
class exahype2::particles::TrajectoryDatabase final : public tarch::database::SnapshotDatabase {
public:
  TrajectoryDatabase(double positionDelta = 1e-8, double timeDelta = 0.0, bool deltasAreRelative = false);
  TrajectoryDatabase(const TrajectoryDatabase& other)            = delete;
  TrajectoryDatabase(TrajectoryDatabase&& other)                 = delete;
  TrajectoryDatabase& operator=(const TrajectoryDatabase& other) = delete;
  TrajectoryDatabase& operator=(TrajectoryDatabase&& other)      = delete;
  ~TrajectoryDatabase() override;

  void addParticleSnapshot(
    const std::pair<int, int>&                   number,
    double                                       timestamp,
    const tarch::la::Vector<DIMENSIONS, double>& x
  );
  void addParticleSnapshot(int number0, int number1, double timestamp, const tarch::la::Vector<DIMENSIONS, double>& x);
  void addParticleSnapshot(
    const std::pair<int, int>&                   number,
    double                                       timestamp,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          numberOfDataEntries,
    const double*                                data
  );
  void addParticleSnapshot(
    int                                          number0,
    int                                          number1,
    double                                       timestamp,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          numberOfDataEntries,
    const double*                                data
  );
  [[nodiscard]] bool isSnapshotDue(
    const std::pair<int, int>&                   number,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timestamp
  );

protected:
  std::string getOutputFileName() const override;
  void prepareOutputStream(std::ostream& file) const override;
  void writeHeader(std::ostream& file) const override;
  bool hasUnwrittenSnapshots() const override;
  void writeUnwrittenSnapshots(std::ostream& file) override;
  void clearStorage() override;
  void resetWriteTracking() override;

private:
  static constexpr double DeltaCutoffThreshold = 1e-6;

  double _positionDelta;
  double _maxPositionDelta{0.0};
  bool   _deltasAreRelative;
  int    _rank{-1};
  int    _numberOfDataPointsPerParticle{0};

  std::map<std::pair<int, int>, int> _writtenSnapshotsPerParticle{};

  struct Entry : Snapshot {
    tarch::la::Vector<DIMENSIONS, double> x;

    Entry(const tarch::la::Vector<DIMENSIONS, double>& position, double timestamp, int numberOfDataEntries);
  };

  std::map<std::pair<int, int>, std::list<Entry>> _data{};

  void writeRow(std::ostream& file, const std::pair<int, int>& number, const Entry& snapshot) const;

  [[nodiscard]] AddSnapshotAction getAction(
    const std::pair<int, int>&                   number,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timestamp
  ) const;
  AddSnapshotAction prepareSnapshot(
    const std::pair<int, int>&                   number,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timestamp
  );
};
