// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

#ifdef PEANO_WITH_NETCDF

namespace tarch::plotter {
  class NetCDFFileWriter;
} // namespace tarch::plotter

/**
 * Writes one UGRID file of the NetCDF format per snapshot. The .nc.series file lists the snapshots
 * of the series.
 */
class tarch::plotter::NetCDFFileWriter final : public tarch::plotter::FileWriter {
public:
  NetCDFFileWriter()                                         = default;
  NetCDFFileWriter(const NetCDFFileWriter& other)            = delete;
  NetCDFFileWriter(NetCDFFileWriter&& other)                 = delete;
  NetCDFFileWriter& operator=(const NetCDFFileWriter& other) = delete;
  NetCDFFileWriter& operator=(NetCDFFileWriter&& other)      = delete;
  ~NetCDFFileWriter() override                               = default;

  void write(const Mesh& mesh, const std::string& outputDirectory, const std::string& filename, double time) override;

  [[nodiscard]] Format getFormat() const override;

private:
  struct Snapshot final {
    double      time{0.0};
    std::string file{};
  };

  void writeSeries(const std::filesystem::path& filePath) const;

  std::vector<Snapshot> _snapshots{};
};

#endif
