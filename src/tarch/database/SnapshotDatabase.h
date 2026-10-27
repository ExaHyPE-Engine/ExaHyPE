// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::database {
  enum class SnapshotRule;
  class SnapshotDatabase;
} // namespace tarch::database

enum class tarch::database::SnapshotRule { Deltas, EveryLaterTimestamp };

class tarch::database::SnapshotDatabase {
public:
  SnapshotDatabase(const SnapshotDatabase&)            = delete;
  SnapshotDatabase(SnapshotDatabase&&)                 = delete;
  SnapshotDatabase& operator=(const SnapshotDatabase&) = delete;
  SnapshotDatabase& operator=(SnapshotDatabase&&)      = delete;
  virtual ~SnapshotDatabase()                          = default;

  void dumpCSVFile();
  void clear(bool lockMutex = true);

  void setOutputFileName(const std::string& filename);
  void setOutputPrecision(int precision);
  void setTimeDeltaBetweenTwoSnapshots(double value);
  void setSnapshotRule(tarch::database::SnapshotRule rule);

protected:
  std::string _fileName{};

  double                        _timeDelta{0.0};
  tarch::database::SnapshotRule _snapshotRule{tarch::database::SnapshotRule::Deltas};

  int  _precision{0};
  bool _csvHeaderWritten{false};

  enum class AddSnapshotAction { Ignore, Append, Replace };

  struct Snapshot {
    double              timestamp{0.0};
    std::vector<double> data{};

    Snapshot(double timestamp, int numberOfDataEntries);
  };

  std::mutex _mutex{};

  explicit SnapshotDatabase(double timeDelta);

  virtual std::string getOutputFileName() const;

  virtual void prepareOutputStream(std::ostream& file) const = 0;
  virtual void writeHeader(std::ostream& file) const         = 0;
  virtual bool hasUnwrittenSnapshots() const                 = 0;
  virtual void writeUnwrittenSnapshots(std::ostream& file)   = 0;
  virtual void clearStorage()                                = 0;
  virtual void resetWriteTracking()                          = 0;
};
