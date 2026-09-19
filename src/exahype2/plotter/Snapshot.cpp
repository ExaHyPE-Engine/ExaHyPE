// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Snapshot.h"

/**
 * Writes the file of this snapshot and the meta file that lists every snapshot of the series.
 * Every piece of the file is complete when the traversal ends.
 */
void exahype2::plotter::Snapshot::writeFile(const std::string& outputDirectory, const std::string& filename) {
  assertion(_fileWriter);
  assertion(_fileWriter->isOpen());
  assertion(_timeSeriesWriter);

  _timeSeriesWriter->writeSnapshotFile(filename, _time, *_fileWriter);
  _timeSeriesWriter->writeTimeSeriesMetaFile(outputDirectory, filename);
}

/**
 * Returns the file writer of the open snapshot.
 */
tarch::plotter::FileWriter& exahype2::plotter::Snapshot::getFileWriter() {
  assertion(_fileWriter);
  return *_fileWriter;
}
