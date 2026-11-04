// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

namespace tarch::plotter {
  class VTUFileWriter;
} // namespace tarch::plotter

/**
 * Writes one .vtu file per rank and one .pvtu file per snapshot. The .pvd file lists the snapshots
 * of the series.
 */
class tarch::plotter::VTUFileWriter final : public tarch::plotter::FileWriter {
public:
  VTUFileWriter()                                      = default;
  VTUFileWriter(const VTUFileWriter& other)            = delete;
  VTUFileWriter(VTUFileWriter&& other)                 = delete;
  VTUFileWriter& operator=(const VTUFileWriter& other) = delete;
  VTUFileWriter& operator=(VTUFileWriter&& other)      = delete;
  ~VTUFileWriter() override                            = default;

  void write(const Mesh& mesh, const std::string& outputDirectory, const std::string& filename, double time) override;
  [[nodiscard]] Format getFormat() const override;

private:
  struct Snapshot final {
    double      time{0.0};
    std::string file{};
  };

  static void writeVTU(const Mesh& mesh, const std::filesystem::path& filePath);
  static void writePVTU(const Mesh& mesh, const std::filesystem::path& filePath, const std::string& snapshotName);
  void writePVD(const std::filesystem::path& filePath) const;

  std::vector<Snapshot> _snapshots{};
};
