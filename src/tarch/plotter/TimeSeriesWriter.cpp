// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSeriesWriter.h"

#include "FileWriter.h"

#include <iomanip>
#include <sstream>

std::string tarch::plotter::TimeSeriesWriter::snapshotName(const std::string& filename, double time) const {
  std::ostringstream oss;
  oss << filename << "-" << std::fixed << std::setprecision(6) << time;
  std::string name = oss.str();
  std::replace(name.begin(), name.end(), '.', '_');
  return name;
}

void tarch::plotter::TimeSeriesWriter::writeSnapshotFile(
  const std::string& filename,
  double             time,
  FileWriter&        fileWriter
) {
  const std::string name = snapshotName(filename, time);
  _snapshots.push_back({time, name + fileWriter.getFileExtension()});
  fileWriter.writeToFile(name, time);
}
