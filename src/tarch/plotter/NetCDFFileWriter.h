// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/FileWriter.h"

#ifdef PEANO_WITH_NETCDF

namespace tarch::plotter {
  class NetCDFFileWriter;
} // namespace tarch::plotter

class tarch::plotter::NetCDFFileWriter final : public tarch::plotter::FileWriter {
public:
  explicit NetCDFFileWriter(const std::string& outputDirectory);
  NetCDFFileWriter(const NetCDFFileWriter&)            = delete;
  NetCDFFileWriter(NetCDFFileWriter&&)                 = delete;
  NetCDFFileWriter& operator=(const NetCDFFileWriter&) = delete;
  NetCDFFileWriter& operator=(NetCDFFileWriter&&)      = delete;
  ~NetCDFFileWriter() override                         = default;

  bool writeToFile(const std::string& filename, const double time) override;

  std::string getFileExtension() const override;

private:
  static void check(const int status, const std::string& operation);
};

#endif
