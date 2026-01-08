// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

namespace tarch::plotter {
  class FileWriter;
  class TimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::TimeSeriesWriter {
public:
  TimeSeriesWriter()          = default;
  virtual ~TimeSeriesWriter() = default;

  /**
   * @brief Adds a new timestep to the series and updates the time series file on disk.
   * Delegates to the file writer to write a new snapshot file and also registers the new snapshot to the time series.
   *
   * @param filename The name of the snapshot file.
   * @param time The simulation time associated with this snapshot.
   * @param fileWriter The corresponding file writer.
   */
  void writeSnapshotFile(const std::string& filename, double time, FileWriter& fileWriter);

  virtual void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const = 0;

protected:
  struct Snapshot {
    double      time;
    std::string filename;
  };

  std::vector<Snapshot> _snapshots;
};
