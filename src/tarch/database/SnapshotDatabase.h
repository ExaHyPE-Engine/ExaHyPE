// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/BooleanSemaphore.h"

#include <ostream>
#include <string>
#include <vector>

namespace tarch::database {
  class SnapshotDatabase;
} // namespace tarch::database

/**
 * Base class for the CSV snapshot databases
 *
 * A snapshot database records a growing series of measurements and appends
 * them, oldest first, to a single CSV file as they arrive. The concrete
 * databases add the storage, the rule that decides whether a new measurement
 * is kept, and the CSV serialisation. The base holds the shared configuration,
 * the incremental file writer and the thread protection.
 */
class tarch::database::SnapshotDatabase {
protected:
  std::string _fileName;

  double _timeDelta;

  int  _precision;
  bool _deltasAreRelative;
  bool _csvHeaderWritten;

  static constexpr double DeltaCutOffThreshold = 1e-6;

  enum class AddSnapshotAction { Ignore, Append, Replace };

  struct Snapshot {
    double              timestamp;
    std::vector<double> data;

    Snapshot(double timestamp_, int numberOfDataEntries);
  };

  int _rank;

  tarch::multicore::BooleanSemaphore _semaphore;

  SnapshotDatabase(double timeDelta, bool deltasAreRelative);

  virtual std::string outputFileName() const;

  virtual void prepareOutputStream(std::ostream& file) const = 0;
  virtual void writeHeader(std::ostream& file) const         = 0;
  virtual bool hasUnwrittenSnapshots() const                 = 0;
  virtual void writeUnwrittenSnapshots(std::ostream& file)   = 0;
  virtual void clearStorage()                                = 0;
  virtual void resetWriteTracking()                          = 0;

public:
  SnapshotDatabase(const SnapshotDatabase&)            = delete;
  SnapshotDatabase(SnapshotDatabase&&)                 = delete;
  SnapshotDatabase& operator=(const SnapshotDatabase&) = delete;
  SnapshotDatabase& operator=(SnapshotDatabase&&)      = delete;
  virtual ~SnapshotDatabase()                          = default;

  void dumpCSVFile();
  void clear(bool lockSemaphore = true);

  void setOutputFileName(const std::string& filename);
  void setOutputPrecision(int precision);
  void setTimeDeltaBetweenTwoSnapshots(double value);
};
