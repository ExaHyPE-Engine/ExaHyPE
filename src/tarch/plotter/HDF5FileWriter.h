// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

#ifdef PEANO_WITH_HDF5

namespace tarch::plotter {
  class HDF5FileWriter;
} // namespace tarch::plotter

class tarch::plotter::HDF5FileWriter final : public FileWriter {
public:
  HDF5FileWriter(const std::string& outputDirectory, const int numberOfSubdomains);
  HDF5FileWriter(const HDF5FileWriter&)            = delete;
  HDF5FileWriter(HDF5FileWriter&&)                 = delete;
  ~HDF5FileWriter() override                       = default;
  HDF5FileWriter& operator=(const HDF5FileWriter&) = delete;
  HDF5FileWriter& operator=(HDF5FileWriter&&)      = delete;

  bool writeToFile(const std::string& filename, const double time) override;

  std::string getFileExtension() const override;

private:
  static int toXdmfType(const int vtkType);
};

#endif
