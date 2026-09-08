// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/TimeSeriesWriter.h"

namespace tarch::plotter {
  class PVDTimeSeriesWriter;
} // namespace tarch::plotter

class tarch::plotter::PVDTimeSeriesWriter final : public tarch::plotter::TimeSeriesWriter {
public:
  PVDTimeSeriesWriter()                                      = default;
  PVDTimeSeriesWriter(const PVDTimeSeriesWriter&)            = delete;
  PVDTimeSeriesWriter(PVDTimeSeriesWriter&&)                 = delete;
  PVDTimeSeriesWriter& operator=(const PVDTimeSeriesWriter&) = delete;
  PVDTimeSeriesWriter& operator=(PVDTimeSeriesWriter&&)      = delete;
  ~PVDTimeSeriesWriter() override                            = default;

  void writeTimeSeriesMetaFile(const std::string& outputDirectory, const std::string& filename) const override;
};
