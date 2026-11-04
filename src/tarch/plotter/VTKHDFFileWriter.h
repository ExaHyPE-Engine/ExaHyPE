// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

namespace tarch::plotter {
  class VTKHDFFileWriter;
} // namespace tarch::plotter

/**
 * Appends one step to a file of the VTKHDF format
 * (https://docs.vtk.org/en/latest/vtk_file_formats/vtkhdf_file_format) per call of write(). The
 * meshes of all ranks form one part of the step.
 */
class tarch::plotter::VTKHDFFileWriter final : public tarch::plotter::FileWriter {
public:
  VTKHDFFileWriter()                                         = default;
  VTKHDFFileWriter(const VTKHDFFileWriter& other)            = delete;
  VTKHDFFileWriter(VTKHDFFileWriter&& other)                 = delete;
  VTKHDFFileWriter& operator=(const VTKHDFFileWriter& other) = delete;
  VTKHDFFileWriter& operator=(VTKHDFFileWriter&& other)      = delete;
  ~VTKHDFFileWriter() override                               = default;

  void write(const Mesh& mesh, const std::string& outputDirectory, const std::string& filename, double time) override;
  [[nodiscard]] Format getFormat() const override;

private:
  std::set<std::string> _createdFiles{};
};
