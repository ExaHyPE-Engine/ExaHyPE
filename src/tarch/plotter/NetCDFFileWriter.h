// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

#ifdef PEANO_WITH_NETCDF

namespace tarch::plotter {
  class NetCDFFileWriter;
} // namespace tarch::plotter

class tarch::plotter::NetCDFFileWriter final: public FileWriter {
public:
  NetCDFFileWriter(const std::string& outputDirectory, int numberOfSubdomains);
  ~NetCDFFileWriter() override = default;

  bool writeToFile(const std::string& filename, double time) override;

  std::string getFileExtension() const override;

private:
  static void check(int status, const std::string& where);
};

#endif
