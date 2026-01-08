// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TimeSeriesWriter.h"

namespace tarch::plotter {
  class XDMFTimeSeriesWriter;
} // namespace tarch::plotter

/**
 * XDMF Time Series Writer.
 * Generates a master XDMF file that links to individual time-step XDMF files
 * using XInclude.
 */
class tarch::plotter::XDMFTimeSeriesWriter final: public TimeSeriesWriter {
public:
  XDMFTimeSeriesWriter()           = default;
  ~XDMFTimeSeriesWriter() override = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
