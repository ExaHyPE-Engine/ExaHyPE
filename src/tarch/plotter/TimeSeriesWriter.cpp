// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSeriesWriter.h"

#include "FileWriter.h"

#include <iomanip>
#include <sstream>

void tarch::plotter::TimeSeriesWriter::
  writeSnapshotFile(const std::string& filename, double time, FileWriter& fileWriter) {
  std::ostringstream oss;
  oss << filename << "-" << std::fixed << std::setprecision(6) << time;

  std::string snapshotName = oss.str();
  std::replace(snapshotName.begin(), snapshotName.end(), '.', '_');

  _snapshots.push_back({time, snapshotName + fileWriter.getFileExtension()});

  fileWriter.writeToFile(snapshotName, time);
}
