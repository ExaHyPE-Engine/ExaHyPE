// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Snapshot.h"

#include "tarch/Assertions.h"

/**
 * Creates the time series writer of the series on the first snapshot and a file writer for
 * this snapshot.
 */
void
exahype2::plotter::Snapshot::openFile(
  const tarch::plotter::Format format,
  const std::string&           outputDirectory,
  const double                 timeStamp
) {
  assertion(std::isfinite(timeStamp), timeStamp);
  if (_timeSeriesWriter == nullptr) {
    _timeSeriesWriter = tarch::plotter::createTimeSeriesWriter(format);
  }
  _fileWriter = tarch::plotter::createFileWriter(format, outputDirectory);
  _time       = timeStamp;
}

/**
 * Writes the file of this snapshot and the meta file that lists every snapshot of the series.
 */
void
exahype2::plotter::Snapshot::writeFile(const std::string& outputDirectory, const std::string& filename) {
  assertion(_fileWriter);
  assertion(_fileWriter->isOpen());
  assertion(_timeSeriesWriter);
  _timeSeriesWriter->writeSnapshotFile(filename, _time, *_fileWriter);
  _timeSeriesWriter->writeTimeSeriesMetaFile(outputDirectory, filename);
}

tarch::plotter::FileWriter&
exahype2::plotter::Snapshot::getFileWriter() {
  assertion(_fileWriter);
  return *_fileWriter;
}
