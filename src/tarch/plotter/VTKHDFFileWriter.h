// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

namespace tarch::plotter {
  class VTKHDFFileWriter;
} // namespace tarch::plotter

/**
 * Writer for the VTKHDF file format (https://docs.vtk.org/en/latest/vtk_file_formats/vtkhdf_file_format).
 *
 * Every writeToFile() call reopens the .vtkhdf file in append mode and adds one
 * step under /VTKHDF/Steps, so isOpen() returns true for the whole run. The
 * connectivity is part-local and stored as int64, and the Offsets array of each
 * part starts with a zero.
 */
class tarch::plotter::VTKHDFFileWriter final : public tarch::plotter::FileWriter {
public:
  explicit VTKHDFFileWriter(const std::string& outputDirectory);
  VTKHDFFileWriter(const VTKHDFFileWriter&)            = delete;
  VTKHDFFileWriter(VTKHDFFileWriter&&)                 = delete;
  VTKHDFFileWriter& operator=(const VTKHDFFileWriter&) = delete;
  VTKHDFFileWriter& operator=(VTKHDFFileWriter&&)      = delete;
  ~VTKHDFFileWriter() override                         = default;

  bool        isOpen() const override;
  bool        writeToFile(const std::string& filename, const double time) override;
  std::string getFileExtension() const override;
};
