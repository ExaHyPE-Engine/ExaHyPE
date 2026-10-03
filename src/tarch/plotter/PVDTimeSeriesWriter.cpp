// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PVDTimeSeriesWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

void
tarch::plotter::PVDTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  const std::filesystem::path path = std::filesystem::path(outputDirectory) / (filename + ".pvd");

  std::ofstream out(path);
  if (not out) {
    tarch::terminateWithError("Failed to open '" + path.string() + "'.");
  }

  out << "<?xml version=\"1.0\"?>\n"
      << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
      << "  <Collection>\n";

  for (const auto& snapshot : _snapshots) {
    out << R"(    <DataSet timestep=")" << tarch::plotter::getExactTimeStamp(snapshot.time)
        << R"(" group="" part="0" file=")" << snapshot.filename << "\"/>\n";
  }

  out << "  </Collection>\n"
      << "</VTKFile>\n";

  if (not out) {
    tarch::terminateWithError("Failed to write '" + path.string() + "'.");
  }
}
