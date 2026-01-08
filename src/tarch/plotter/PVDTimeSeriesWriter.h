// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TimeSeriesWriter.h"

namespace tarch::plotter {
  class PVDTimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::PVDTimeSeriesWriter final: public TimeSeriesWriter {
public:
  PVDTimeSeriesWriter()           = default;
  ~PVDTimeSeriesWriter() override = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
