// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

namespace tarch::plotter {
  class VTUFileWriter;
} // namespace tarch::plotter

class tarch::plotter::VTUFileWriter final: public FileWriter {
public:
  VTUFileWriter(const std::string& outputDirectory, int numberOfSubdomains);
  ~VTUFileWriter() override = default;

  bool writeToFile(const std::string& filename, double time) override;

  std::string getFileExtension() const override;

private:
  void writeVTU(const std::string& filename);
  void writePVTU(const std::string& filename);
};
