// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"

namespace peano4::particles {
  class TrajectoryDatabase;
} // namespace peano4::particles

/**
 * Records a series of snapshots per particle, each snapshot holding a position
 * and optional data, and appends them to a CSV file. A pair of integers
 * identifies a particle. The threads of a traversal add snapshots at the same
 * time, so addParticleSnapshot() locks _mutex.
 */
class peano4::particles::TrajectoryDatabase final : public tarch::database::SnapshotDatabase {
public:
  TrajectoryDatabase(double positionDelta = 1e-8, double timeDelta = 0.0, bool deltasAreRelative = false);
  ~TrajectoryDatabase() override;

  void setPositionDeltaBetweenTwoSnapshots(double value, bool deltasAreRelative = false);

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

protected:
  std::string outputFileName() const override;
  void        prepareOutputStream(std::ostream& file) const override;
  void        writeHeader(std::ostream& file) const override;
  bool        hasUnwrittenSnapshots() const override;
  void        writeUnwrittenSnapshots(std::ostream& file) override;
  void        clearStorage() override;
  void        resetWriteTracking() override;

private:
  double _positionDelta;
  double _maxPositionDelta{0.0};
  int    _numberOfDataPointsPerParticle{0};

  std::map<std::pair<int, int>, int> _writtenSnapshotsPerParticle;

  struct Entry : Snapshot {
    tarch::la::Vector<DIMENSIONS, double> x;

    Entry(const tarch::la::Vector<DIMENSIONS, double>& position, double timestamp, int numberOfDataEntries);
  };

  std::map<std::pair<int, int>, std::list<Entry>> _data;

  void writeRow(std::ostream& file, const std::pair<int, int>& number, const Entry& snapshot) const;

  AddSnapshotAction getAction(
    const std::pair<int, int>&                   number,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timestamp
  );
};
