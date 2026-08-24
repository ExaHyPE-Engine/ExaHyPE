// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFTimeSeriesWriter.h"

void tarch::plotter::VTKHDFTimeSeriesWriter::writeTimeSeriesMetaFile(
  [[maybe_unused]] const std::string& outputDirectory,
  [[maybe_unused]] const std::string& filename
) const {}

std::string tarch::plotter::VTKHDFTimeSeriesWriter::snapshotName(
  const std::string&            filename,
  [[maybe_unused]] const double time
) const {
  return filename;
}
