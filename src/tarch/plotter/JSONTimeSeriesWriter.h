// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/TimeSeriesWriter.h"

namespace tarch::plotter {
  class JSONTimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::JSONTimeSeriesWriter final : public tarch::plotter::TimeSeriesWriter {
public:
  JSONTimeSeriesWriter()                                       = default;
  JSONTimeSeriesWriter(const JSONTimeSeriesWriter&)            = delete;
  JSONTimeSeriesWriter(JSONTimeSeriesWriter&&)                 = delete;
  JSONTimeSeriesWriter& operator=(const JSONTimeSeriesWriter&) = delete;
  JSONTimeSeriesWriter& operator=(JSONTimeSeriesWriter&&)      = delete;
  ~JSONTimeSeriesWriter() override                             = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
