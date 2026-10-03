// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONTimeSeriesWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

void
tarch::plotter::JSONTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  // ParaView picks the reader of a file series by the extension before .series, such as
  // name.nc.series.
  std::string dataExtension;
  if (not _snapshots.empty()) {
    const std::string& firstFile = _snapshots.front().filename;
    const std::size_t  dot       = firstFile.find_last_of('.');
    if (dot != std::string::npos) {
      dataExtension = firstFile.substr(dot);
    }
  }

  const std::filesystem::path path = std::filesystem::path(outputDirectory) / (filename + dataExtension + ".series");

  std::ofstream out(path);
  if (not out) {
    tarch::terminateWithError("Failed to open '" + path.string() + "'.");
  }

  out << "{\n"
      << "  \"file-series-version\" : \"1.0\",\n"
      << "  \"files\" : [\n";

  for (size_t i = 0; i < _snapshots.size(); ++i) {
    const auto& entry = _snapshots[i];
    out << R"(    { "name" : ")" << entry.filename << R"(", "time" : )" << tarch::plotter::getExactTimeStamp(entry.time)
        << " }";

    if (i < _snapshots.size() - 1) {
      out << ",";
    }
    out << "\n";
  }

  out << "  ]\n"
      << "}\n";
  if (not out) {
    tarch::terminateWithError("Failed to write '" + path.string() + "'.");
  }
}
