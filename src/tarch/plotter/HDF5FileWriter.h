// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

#ifdef PEANO_WITH_HDF5

namespace tarch::plotter {
  class HDF5FileWriter;
} // namespace tarch::plotter

/**
 * Writes one .h5 file and one .xmf file per snapshot. The .xmf file of the series embeds the grids
 * of all snapshots.
 */
class tarch::plotter::HDF5FileWriter final : public tarch::plotter::FileWriter {
public:
  HDF5FileWriter()                                       = default;
  HDF5FileWriter(const HDF5FileWriter& other)            = delete;
  HDF5FileWriter(HDF5FileWriter&& other)                 = delete;
  HDF5FileWriter& operator=(const HDF5FileWriter& other) = delete;
  HDF5FileWriter& operator=(HDF5FileWriter&& other)      = delete;
  ~HDF5FileWriter() override                             = default;

  void write(const Mesh& mesh, const std::string& outputDirectory, const std::string& filename, double time) override;
  [[nodiscard]] Format getFormat() const override;

private:
  struct Snapshot final {
    double      time{0.0};
    std::string file{};
    std::string grid{};
  };

  void writeSeries(const std::filesystem::path& filePath) const;

  std::vector<Snapshot> _snapshots{};
};

#endif
