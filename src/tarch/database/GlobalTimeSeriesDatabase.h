// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/database/SnapshotDatabase.h"

namespace tarch::database {
  class GlobalTimeSeriesDatabase;
} // namespace tarch::database

class tarch::database::GlobalTimeSeriesDatabase final : public tarch::database::SnapshotDatabase {
public:
  GlobalTimeSeriesDatabase(double timeDelta = 0.0, bool deltasAreRelative = false);
  ~GlobalTimeSeriesDatabase() override;

  void setDataName(const std::string& dataName);

  void addGlobalSnapshot(double timestamp, int numberOfDataEntries, const double* data);

protected:
  void prepareOutputStream(std::ostream& file) const override;
  void writeHeader(std::ostream& file) const override;
  bool hasUnwrittenSnapshots() const override;
  void writeUnwrittenSnapshots(std::ostream& file) override;
  void clearStorage() override;
  void resetWriteTracking() override;

private:
  std::string _dataName{" data "};
  int         _numberOfGlobalDataPoints{0};
  int         _numberOfEntriesWrittenToFile{0};

  std::list<Snapshot> _data{};

  void              writeRow(std::ostream& file, const Snapshot& snapshot) const;
  AddSnapshotAction getAction(double timestamp);
};
