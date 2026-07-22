// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "XDMFTimeSeriesWriter.h"

#include "tarch/mpi/Rank.h"

namespace {
  /**
   * Extract the body of the single <Grid> element of a self-contained per-step
   * XDMF file, i.e. everything between the opening "<Grid ...>" tag and the
   * closing "</Grid>". That body (the Topology, Geometry and Attribute elements
   * with their DataItem references into the HDF5 file) is what gets inlined into
   * the temporal collection.
   */
  std::string extractGridBody(const std::filesystem::path& perStepFile) {
    std::ifstream in(perStepFile);
    if (not in) {
      throw std::runtime_error("Failed to read per-step XDMF file '" + perStepFile.string() + "'");
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string content = buffer.str();

    const std::size_t gridOpen   = content.find("<Grid");
    const std::size_t openTagEnd = gridOpen == std::string::npos ? std::string::npos : content.find('>', gridOpen);
    const std::size_t gridClose  = content.rfind("</Grid>");
    if (gridOpen == std::string::npos or openTagEnd == std::string::npos or gridClose == std::string::npos or
        gridClose <= openTagEnd) {
      throw std::runtime_error("Per-step XDMF file '" + perStepFile.string() + "' has no <Grid> element to inline.");
    }
    return content.substr(openTagEnd + 1, gridClose - (openTagEnd + 1));
  }
} // namespace

void tarch::plotter::XDMFTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  std::filesystem::path directory(outputDirectory);
  std::filesystem::path p = directory / (filename + ".xmf");

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  out << "<?xml version=\"1.0\" ?>\n"
      << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
      << "<Xdmf Version=\"2.0\">\n"
      << " <Domain>\n"
      << "  <Grid Name=\"TimeSeries\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";

  for (const auto& snapshot : _snapshots) {
    const std::string gridBody = extractGridBody(directory / snapshot.filename);
    out << "   <Grid Name=\"Step_" << snapshot.time << "\" GridType=\"Uniform\">\n"
        << "    <Time Value=\"" << snapshot.time << "\"/>\n"
        << gridBody << "   </Grid>\n";
  }

  out << "  </Grid>\n"
      << " </Domain>\n"
      << "</Xdmf>\n";
}
