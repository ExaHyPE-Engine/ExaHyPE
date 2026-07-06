// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFTimeSeriesWriter.h"

void tarch::plotter::VTKHDFTimeSeriesWriter::writeTimeSeriesMetaFile(const std::string&, const std::string&) const {}

std::string tarch::plotter::VTKHDFTimeSeriesWriter::snapshotName(const std::string& filename, double) const {
  return filename;
}
