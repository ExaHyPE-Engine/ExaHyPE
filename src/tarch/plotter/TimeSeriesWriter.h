// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::plotter {
  class FileWriter;
  class TimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::TimeSeriesWriter {
public:
  TimeSeriesWriter()                                   = default;
  TimeSeriesWriter(const TimeSeriesWriter&)            = delete;
  TimeSeriesWriter(TimeSeriesWriter&&)                 = delete;
  virtual ~TimeSeriesWriter()                          = default;
  TimeSeriesWriter& operator=(const TimeSeriesWriter&) = delete;
  TimeSeriesWriter& operator=(TimeSeriesWriter&&)      = delete;

  /**
   * @brief Adds a new timestep to the series and updates the time series file on disk.
   * Delegates to the file writer to write a new snapshot file and also registers the new snapshot to the time series.
   *
   * @param filename The name of the snapshot file.
   * @param time The simulation time associated with this snapshot.
   * @param fileWriter The corresponding file writer.
   */
  void writeSnapshotFile(const std::string& filename, const double time, FileWriter& fileWriter);

  virtual void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const = 0;

protected:
  // Per-snapshot file name passed to the file writer. The default stamps the
  // time into the name (<filename>-<time>) so each snapshot is a distinct file.
  // A writer that keeps all time steps in one file (VTKHDF) overrides this to
  // return the stable base name unchanged.
  virtual std::string snapshotName(const std::string& filename, const double time) const;

  struct Snapshot {
    double      time;
    std::string filename;
  };

  std::vector<Snapshot> _snapshots;
};
