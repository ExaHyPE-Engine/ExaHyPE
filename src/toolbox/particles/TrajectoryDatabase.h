// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"

namespace toolbox::particles {
  class TrajectoryDatabase;
} // namespace toolbox::particles

/**
 * A particle trajectory database
 *
 * Records, per particle, a series of snapshots (position plus optional data)
 * and appends them to a CSV file. A particle is identified by a pair of
 * integers. Particles are distributed over the ranks, so each rank writes its
 * own file. As snapshots are dumped from the multithreaded grid traversal, the
 * database is fed concurrently and thus locked.
 */
class toolbox::particles::TrajectoryDatabase final : public tarch::database::SnapshotDatabase {
private:
  double _positionDelta;
  double _maxPositionDelta;
  int    _numberOfDataPointsPerParticle;

  std::map<std::pair<int, int>, int> _writtenSnapshotsPerParticle;

  struct Entry : Snapshot {
    tarch::la::Vector<DIMENSIONS, double> x;

    Entry(const tarch::la::Vector<DIMENSIONS, double>& x_, double timestamp_, int numberOfDataEntries);
  };

  std::map<std::pair<int, int>, std::list<Entry>> _data;

  void writeRow(std::ostream& file, const std::pair<int, int>& number, const Entry& snapshot) const;

  AddSnapshotAction
  getAction(const std::pair<int, int>& number, const tarch::la::Vector<DIMENSIONS, double>& x, double timestamp);

protected:
  std::string outputFileName() const override;
  void        prepareOutputStream(std::ostream& file) const override;
  void        writeHeader(std::ostream& file) const override;
  bool        hasUnwrittenSnapshots() const override;
  void        writeUnwrittenSnapshots(std::ostream& file) override;
  void        clearStorage() override;
  void        resetWriteTracking() override;

public:
  TrajectoryDatabase(double positionDelta = 1e-8, double timeDelta = 0.0, bool deltasAreRelative = false);
  ~TrajectoryDatabase();

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
};
