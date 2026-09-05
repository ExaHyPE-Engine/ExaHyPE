// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CheckpointFile.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/GridCheckpoint.h"
#include "tarch/Assertions.h"

peano4::checkpoint::CheckpointFile& peano4::checkpoint::CheckpointFile::getInstance() {
  static peano4::checkpoint::CheckpointFile instance;
  return instance;
}

std::string peano4::checkpoint::CheckpointFile::getPath(const std::string& directory, const int numberOfCheckpoint) {
  assertion(numberOfCheckpoint >= 0, numberOfCheckpoint);
  return directory + "/checkpoint-" + std::to_string(numberOfCheckpoint) + ".h5";
}

void peano4::checkpoint::CheckpointFile::open(const std::string& directory) {
  assertion(not directory.empty());
  if (_numberOfWriters == 0) {
    const std::string path = peano4::checkpoint::CheckpointFile::getPath(directory, _numberOfCheckpoints);
    peano4::checkpoint::createDirectoryFor(path);
    // Every rank joins the file, so a rank without records for one writer
    // still takes part in that writer's collective dataset calls.
    _file = tarch::plotter::HDF5File::openForDataHoldingRanks(path, true);
  }
  _numberOfWriters++;
}

tarch::plotter::HDF5File& peano4::checkpoint::CheckpointFile::getFile() {
  assertion(_file != nullptr);
  return *_file;
}

void peano4::checkpoint::CheckpointFile::close() {
  assertion(_numberOfWriters > 0, _numberOfWriters);
  _numberOfWriters--;
  if (_numberOfWriters == 0) {
    _file.reset();
    _numberOfCheckpoints++;
  }
}

int peano4::checkpoint::CheckpointFile::getNumberOfCheckpoints() const { return _numberOfCheckpoints; }

#endif
