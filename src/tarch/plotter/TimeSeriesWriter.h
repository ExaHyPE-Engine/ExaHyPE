// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::plotter {
  class FileWriter;
  class TimeSeriesWriter;

  [[nodiscard]] std::string getExactTimeStamp(double time);
} // namespace tarch::plotter

class tarch::plotter::TimeSeriesWriter {
public:
  TimeSeriesWriter()                                   = default;
  TimeSeriesWriter(const TimeSeriesWriter&)            = delete;
  TimeSeriesWriter(TimeSeriesWriter&&)                 = delete;
  virtual ~TimeSeriesWriter()                          = default;
  TimeSeriesWriter& operator=(const TimeSeriesWriter&) = delete;
  TimeSeriesWriter& operator=(TimeSeriesWriter&&)      = delete;

  void writeSnapshotFile(const std::string& filename, double time, FileWriter& fileWriter);

  virtual void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const = 0;

protected:
  [[nodiscard]] virtual std::string getSnapshotName(const std::string& filename, double time) const;

  struct Snapshot {
    double      time;
    std::string filename;
  };

  std::vector<Snapshot> _snapshots;
};
