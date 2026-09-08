// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/TimeSeriesWriter.h"

namespace tarch::plotter {
  class XDMFTimeSeriesWriter;
} // namespace tarch::plotter

/**
 * Generates a master XDMF file by embedding each time-step file's `<Grid>` body.
 */
class tarch::plotter::XDMFTimeSeriesWriter final : public tarch::plotter::TimeSeriesWriter {
public:
  XDMFTimeSeriesWriter()                                       = default;
  XDMFTimeSeriesWriter(const XDMFTimeSeriesWriter&)            = delete;
  XDMFTimeSeriesWriter(XDMFTimeSeriesWriter&&)                 = delete;
  XDMFTimeSeriesWriter& operator=(const XDMFTimeSeriesWriter&) = delete;
  XDMFTimeSeriesWriter& operator=(XDMFTimeSeriesWriter&&)      = delete;
  ~XDMFTimeSeriesWriter() override                             = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
