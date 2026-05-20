// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TimeSeriesWriter.h"

namespace tarch::plotter {
  class JSONTimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::JSONTimeSeriesWriter: public TimeSeriesWriter {
public:
  JSONTimeSeriesWriter()           = default;
  ~JSONTimeSeriesWriter() override = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
