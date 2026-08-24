// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TimeSeriesWriter.h"

namespace tarch::plotter {
  class VTKHDFTimeSeriesWriter;
} // namespace tarch::plotter

/**
 * Time-series writer for the VTKHDF format. A VTKHDFFileWriter keeps every time
 * step in one growing .vtkhdf file, so that single file already is the temporal
 * collection: there is no separate index file to emit, and the per-step file
 * name must stay stable (no time stamp) so each traversal appends to the same
 * file.
 */
class tarch::plotter::VTKHDFTimeSeriesWriter : public tarch::plotter::TimeSeriesWriter {
public:
  VTKHDFTimeSeriesWriter()                                         = default;
  VTKHDFTimeSeriesWriter(const VTKHDFTimeSeriesWriter&)            = delete;
  VTKHDFTimeSeriesWriter(VTKHDFTimeSeriesWriter&&)                 = delete;
  ~VTKHDFTimeSeriesWriter() override                               = default;
  VTKHDFTimeSeriesWriter& operator=(const VTKHDFTimeSeriesWriter&) = delete;
  VTKHDFTimeSeriesWriter& operator=(VTKHDFTimeSeriesWriter&&)      = delete;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;

protected:
  std::string snapshotName(const std::string& filename, const double time) const override;
};
