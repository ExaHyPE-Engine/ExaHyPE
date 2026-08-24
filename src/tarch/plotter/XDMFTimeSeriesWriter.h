// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TimeSeriesWriter.h"

namespace tarch::plotter {
  class XDMFTimeSeriesWriter;
} // namespace tarch::plotter

/**
 * Generates a master XDMF file by embedding each time-step file's `<Grid>` body.
 */
class tarch::plotter::XDMFTimeSeriesWriter final : public TimeSeriesWriter {
public:
  XDMFTimeSeriesWriter()                                       = default;
  XDMFTimeSeriesWriter(const XDMFTimeSeriesWriter&)            = delete;
  XDMFTimeSeriesWriter(XDMFTimeSeriesWriter&&)                 = delete;
  ~XDMFTimeSeriesWriter() override                             = default;
  XDMFTimeSeriesWriter& operator=(const XDMFTimeSeriesWriter&) = delete;
  XDMFTimeSeriesWriter& operator=(XDMFTimeSeriesWriter&&)      = delete;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
