// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

#ifdef PEANO_WITH_NETCDF

namespace tarch::plotter {
  class NetCDFFileWriter;
} // namespace tarch::plotter

class tarch::plotter::NetCDFFileWriter final : public FileWriter {
public:
  NetCDFFileWriter(const std::string& outputDirectory, const int numberOfSubdomains);
  NetCDFFileWriter(const NetCDFFileWriter&)            = delete;
  NetCDFFileWriter(NetCDFFileWriter&&)                 = delete;
  ~NetCDFFileWriter() override                         = default;
  NetCDFFileWriter& operator=(const NetCDFFileWriter&) = delete;
  NetCDFFileWriter& operator=(NetCDFFileWriter&&)      = delete;

  bool writeToFile(const std::string& filename, const double time) override;

  std::string getFileExtension() const override;

private:
  static void check(const int status, const std::string& operation);
};

#endif
