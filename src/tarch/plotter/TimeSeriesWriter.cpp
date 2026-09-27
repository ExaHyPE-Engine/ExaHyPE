// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSeriesWriter.h"

#include "tarch/plotter/FileWriter.h"

std::string
tarch::plotter::TimeSeriesWriter::getSnapshotName(const std::string& filename, const double time) const {
  std::ostringstream stream;
  stream << filename << "-" << std::fixed << std::setprecision(6) << time;
  std::string name = stream.str();
  std::replace(name.begin(), name.end(), '.', '_');
  return name;
}

void
tarch::plotter::TimeSeriesWriter::writeSnapshotFile(
  const std::string& filename,
  const double       time,
  FileWriter&        fileWriter
) {
  const std::string name = getSnapshotName(filename, time);
  _snapshots.push_back({time, name + fileWriter.getFileExtension()});
  fileWriter.writeToFile(name, time);
}
