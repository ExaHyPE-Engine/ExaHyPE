// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSeriesWriter.h"

#include "tarch/Assertions.h"
#include "tarch/plotter/FileWriter.h"

/**
 * Returns the shortest decimal representation of time that reads back as the same double.
 */
std::string
tarch::plotter::getExactTimeStamp(const double time) {
  std::array<char, 32> buffer{};
  const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), time);
  assertion(error == std::errc(), time);
  return std::string(buffer.data(), end);
}

/**
 * Returns filename followed by the time stamp, so every snapshot gets a file of its own. A writer
 * that keeps all time steps in one file returns filename unchanged.
 */
std::string
tarch::plotter::TimeSeriesWriter::getSnapshotName(const std::string& filename, const double time) const {
  std::ostringstream stream;
  stream << filename << "-" << std::fixed << std::setprecision(6) << time;
  std::string name = stream.str();
  std::replace(name.begin(), name.end(), '.', '_');
  return name;
}

/**
 * Writes the snapshot at time through fileWriter and adds it to the series.
 */
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
