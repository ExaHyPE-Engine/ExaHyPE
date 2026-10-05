// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::plotter {
  class FileWriter;

  /**
   * Lists the file formats of the plots.
   */
  enum class Format { VTU, HDF5, NetCDF, VTKHDF };

  [[nodiscard]] Format parseFormat(const std::string& name);
  [[nodiscard]] std::string toString(Format format);
  [[nodiscard]] bool isAvailable(Format format);
  [[nodiscard]] Format getDefaultFormat();
  [[nodiscard]] std::unique_ptr<FileWriter> createFileWriter(Format format);
} // namespace tarch::plotter
