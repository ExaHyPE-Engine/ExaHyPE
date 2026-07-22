// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PVDTimeSeriesWriter.h"

#include "tarch/mpi/Rank.h"

void tarch::plotter::PVDTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  std::filesystem::path p(outputDirectory);
  p /= (filename + ".pvd");

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  out
    << "<?xml version=\"1.0\"?>\n"
    << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
    << "  <Collection>\n";

  out << std::setprecision(15); // Ensure time is accurate

  // Write all snapshots
  for (const auto& s : _snapshots) {
    out << "    <DataSet timestep=\"" << s.time << "\" group=\"\" part=\"0\" file=\"" << s.filename << "\"/>\n";
  }

  out << "  </Collection>\n"
      << "</VTKFile>\n";
}
