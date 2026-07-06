// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"

#include <string>

namespace tarch::plotter {
  class VTKHDFFileWriter;
} // namespace tarch::plotter

/**
 * Writer for the VTKHDF file format (https://docs.vtk.org/en/latest/vtk_file_formats/vtkhdf_file_format).
 *
 * Unlike the per-snapshot writers (HDF5/XDMF, VTU, NetCDF), this writer keeps
 * every time step of a plotter in one growing <name>.vtkhdf file: each
 * PlotSolution traversal appends a step to the transient VTKHDF UnstructuredGrid
 * layout (/VTKHDF + /VTKHDF/Steps). ParaView/VTK read it natively as a time
 * series, so there is no XML sidecar.
 *
 * To fit the per-traversal observer lifecycle (a fresh FileWriter is created
 * each traversal), the file is reopened in append mode each writeToFile() call;
 * the only cross-traversal state - which paths have been created this run - is
 * static. isOpen() always returns true (the file is logically open for the whole
 * run) and the one-shot _written latch of the per-snapshot writers is not used.
 *
 * Reuses FileWriter's geometry/data accumulation, schema reduction and parallel
 * layout; the VTKHDF connectivity is part-local and stored as int64, with an
 * Offsets array carrying a leading 0 per part.
 */
class tarch::plotter::VTKHDFFileWriter : public tarch::plotter::FileWriter {
public:
  VTKHDFFileWriter(const std::string& outputDirectory, int numberOfSubdomains);

  bool        isOpen() const override;
  bool        writeToFile(const std::string& filename, double time) override;
  std::string getFileExtension() const override;
};
