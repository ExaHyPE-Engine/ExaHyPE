// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONTimeSeriesWriter.h"

#include "tarch/mpi/Rank.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void tarch::plotter::JSONTimeSeriesWriter::writeTimeSeriesMetaFile(
  const std::string& outputDirectory,
  const std::string& filename
) const {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  fs::path p(outputDirectory);
  p /= (filename + ".series");

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  out
    << "{\n"
    << "  \"file-series-version\" : \"1.0\",\n"
    << "  \"files\" : [\n";

  for (size_t i = 0; i < _snapshots.size(); ++i) {
    const auto& entry = _snapshots[i];
    out << "    { \"name\" : \"" << entry.filename << "\", \"time\" : " << entry.time << " }";

    if (i < _snapshots.size() - 1) {
      out << ",";
    }
    out << "\n";
  }

  out << "  ]\n"
      << "}\n";
}
