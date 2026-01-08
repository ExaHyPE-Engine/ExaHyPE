// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "XDMFTimeSeriesWriter.h"

#include "tarch/mpi/Rank.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void tarch::plotter::XDMFTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  fs::path p(outputDirectory);
  p /= (filename + ".xmf");

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  // Write XDMF Temporal Collection
  out
    << "<?xml version=\"1.0\" ?>\n"
    << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
    << "<Xdmf Version=\"2.0\" xmlns:xi=\"http://www.w3.org/2001/XInclude\">\n"
    << " <Domain>\n"
    << "  <Grid Name=\"TimeSeries\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";

  for (const auto& snapshot : _snapshots) {
    out
      << "   <Grid Name=\"Step_" << snapshot.time << "\" GridType=\"Uniform\">\n"
      << "    <Time Value=\"" << snapshot.time << "\"/>\n"
      << "    <xi:include href=\"" << snapshot.filename << "\" xpointer=\"xpointer(//Xdmf/Domain/Grid/*)\"/>\n"
      << "   </Grid>\n";
  }

  out
    << "  </Grid>\n"
    << " </Domain>\n"
    << "</Xdmf>\n";
}
