// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "XDMFTimeSeriesWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

namespace {
  /**
   * Returns the content of the only Grid element of a per-step XDMF file, which the temporal
   * collection inlines.
   */
  std::string extractGridBody(const std::filesystem::path& perStepFile) {
    const std::ifstream input(perStepFile);
    if (not input) {
      tarch::terminateWithError("Failed to read '" + perStepFile.string() + "'.");
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string content = buffer.str();

    const std::size_t gridOpen   = content.find("<Grid");
    const std::size_t openTagEnd = gridOpen == std::string::npos ? std::string::npos : content.find('>', gridOpen);
    const std::size_t gridClose  = content.rfind("</Grid>");
    if (
      gridOpen == std::string::npos or openTagEnd == std::string::npos or gridClose == std::string::npos
      or gridClose <= openTagEnd
    ) {
      tarch::terminateWithError("Per-step XDMF file '" + perStepFile.string() + "' has no <Grid> element to inline.");
    }
    return content.substr(openTagEnd + 1, gridClose - (openTagEnd + 1));
  }
} // namespace

void
tarch::plotter::XDMFTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  const std::filesystem::path directory(outputDirectory);
  const std::filesystem::path path = directory / (filename + ".xmf");

  std::ofstream out(path);
  if (not out) {
    tarch::terminateWithError("Failed to open '" + path.string() + "'.");
  }

  out << "<?xml version=\"1.0\" ?>\n"
      << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
      << "<Xdmf Version=\"2.0\">\n"
      << " <Domain>\n"
      << "  <Grid Name=\"TimeSeries\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";

  for (const auto& snapshot : _snapshots) {
    const std::string gridBody  = extractGridBody(directory / snapshot.filename);
    const std::string timeStamp = tarch::plotter::getExactTimeStamp(snapshot.time);
    out << "   <Grid Name=\"Step_" << timeStamp << "\" GridType=\"Uniform\">\n"
        << "    <Time Value=\"" << timeStamp << "\"/>\n"
        << gridBody << "   </Grid>\n";
  }

  out << "  </Grid>\n"
      << " </Domain>\n"
      << "</Xdmf>\n";

  if (not out) {
    tarch::terminateWithError("Failed to write '" + path.string() + "'.");
  }
}
