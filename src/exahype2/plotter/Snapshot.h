// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"
#include "tarch/plotter/TimeSeriesWriter.h"

namespace exahype2::plotter {
  class Snapshot;
} // namespace exahype2::plotter

/**
 * Holds the writers of one plot series: One file writer per snapshot and one time series
 * writer for the whole series.
 */
class exahype2::plotter::Snapshot final {
public:
  Snapshot()                           = default;
  Snapshot(const Snapshot&)            = delete;
  Snapshot(Snapshot&&)                 = delete;
  Snapshot& operator=(const Snapshot&) = delete;
  Snapshot& operator=(Snapshot&&)      = delete;
  ~Snapshot()                          = default;

  void openFile(tarch::plotter::Format format, const std::string& outputDirectory, double timeStamp);

  void writeFile(const std::string& outputDirectory, const std::string& filename);

  [[nodiscard]] tarch::plotter::FileWriter& getFileWriter();

private:
  double                                            _time{0.0};
  std::unique_ptr<tarch::plotter::FileWriter>       _fileWriter{};
  std::unique_ptr<tarch::plotter::TimeSeriesWriter> _timeSeriesWriter{};
};
