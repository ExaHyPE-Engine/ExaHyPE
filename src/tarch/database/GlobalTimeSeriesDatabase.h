// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/database/SnapshotDatabase.h"

#include <list>
#include <ostream>
#include <string>

namespace tarch::database {
  class GlobalTimeSeriesDatabase;
} // namespace tarch::database

/**
 * A global time-series database
 *
 * Records domain-global data, identified solely by its time stamp, and appends
 * it to a single CSV file. The typical client is the solver error norm, which
 * is reduced across all ranks and thus written only once.
 */
class tarch::database::GlobalTimeSeriesDatabase final : public tarch::database::SnapshotDatabase {
private:
  std::string _dataName;
  int         _numberOfGlobalDataPoints;
  int         _numberOfEntriesWrittenToFile;

  std::list<Snapshot> _data;

  void              writeRow(std::ostream& file, const Snapshot& snapshot) const;
  AddSnapshotAction getAction(double timestamp);

protected:
  void prepareOutputStream(std::ostream& file) const override;
  void writeHeader(std::ostream& file) const override;
  bool hasUnwrittenSnapshots() const override;
  void writeUnwrittenSnapshots(std::ostream& file) override;
  void clearStorage() override;
  void resetWriteTracking() override;

public:
  GlobalTimeSeriesDatabase(double timeDelta = 0.0, bool deltasAreRelative = false);
  ~GlobalTimeSeriesDatabase();

  void setDataName(const std::string& dataname);

  void addGlobalSnapshot(double timestamp, int numberOfDataEntries, const double* data);
};
