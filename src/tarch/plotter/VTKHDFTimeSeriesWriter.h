// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/TimeSeriesWriter.h"

namespace tarch::plotter {
  class VTKHDFTimeSeriesWriter;
} // namespace tarch::plotter

/**
 * Writes no index file, because one .vtkhdf file stores every time step. The snapshot name has no
 * time stamp, so every snapshot appends to the same file.
 */
class tarch::plotter::VTKHDFTimeSeriesWriter final : public tarch::plotter::TimeSeriesWriter {
public:
  VTKHDFTimeSeriesWriter()                                         = default;
  VTKHDFTimeSeriesWriter(const VTKHDFTimeSeriesWriter&)            = delete;
  VTKHDFTimeSeriesWriter(VTKHDFTimeSeriesWriter&&)                 = delete;
  VTKHDFTimeSeriesWriter& operator=(const VTKHDFTimeSeriesWriter&) = delete;
  VTKHDFTimeSeriesWriter& operator=(VTKHDFTimeSeriesWriter&&)      = delete;
  ~VTKHDFTimeSeriesWriter() override                               = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;

protected:
  [[nodiscard]] std::string getSnapshotName(const std::string& filename, double time) const override;
};
