// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

#ifdef WITH_HDF5

namespace tarch::plotter {
  class HDF5FileWriter;
} // namespace tarch::plotter

class tarch::plotter::HDF5FileWriter final: public FileWriter {
public:
  HDF5FileWriter(const std::string& outputDirectory, int numberOfSubdomains);
  ~HDF5FileWriter() override = default;

  bool writeToFile(const std::string& filename, double time) override;

  std::string getFileExtension() const override;

private:
  static int toXdmfType(int vtkType);
};

#endif
