// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::database {
  class SnapshotDatabase;
} // namespace tarch::database

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

protected:
  std::string _fileName{};

  double _timeDelta{0.0};

  int  _precision{0};
  bool _deltasAreRelative{false};
  bool _csvHeaderWritten{false};

  static constexpr double DeltaCutoffThreshold = 1e-6;

  enum class AddSnapshotAction { Ignore, Append, Replace };

  struct Snapshot {
    double              timestamp{0.0};
    std::vector<double> data{};

    Snapshot(double timestamp, int numberOfDataEntries);
  };

  int _rank{-1};

  std::mutex _mutex{};

  SnapshotDatabase(double timeDelta, bool deltasAreRelative);

  virtual std::string getOutputFileName() const;

  virtual void prepareOutputStream(std::ostream& file) const = 0;
  virtual void writeHeader(std::ostream& file) const         = 0;
  virtual bool hasUnwrittenSnapshots() const                 = 0;
  virtual void writeUnwrittenSnapshots(std::ostream& file)   = 0;
  virtual void clearStorage()                                = 0;
  virtual void resetWriteTracking()                          = 0;
};
