// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

#include "tarch/plotter/HDF5File.h"

namespace peano4::checkpoint {
  class CheckpointFile;
} // namespace peano4::checkpoint

/**
 * Holds the one file a checkpoint writes.
 *
 * The grid, every solver and every particle set of a checkpoint write into a
 * group of this file, so a run writes one file per checkpoint whatever it
 * couples. Every writer calls open() before it writes and close() when it has
 * written: The first open() creates the file, the last close() closes it and
 * counts the checkpoint up. Both calls are collective over every rank, and
 * every rank makes them in the same order.
 */
class peano4::checkpoint::CheckpointFile final {
public:
  CheckpointFile(const CheckpointFile&)            = delete;
  CheckpointFile(CheckpointFile&&)                 = delete;
  CheckpointFile& operator=(const CheckpointFile&) = delete;
  CheckpointFile& operator=(CheckpointFile&&)      = delete;
  ~CheckpointFile()                                = default;

  static CheckpointFile& getInstance();

  /** Returns the path the checkpoint with this number takes in directory. */
  [[nodiscard]] static std::string getPath(const std::string& directory, int numberOfCheckpoint);

  void open(const std::string& directory);

  [[nodiscard]] tarch::plotter::HDF5File& getFile();

  void close();

  /** Number of checkpoints this run has written. */
  [[nodiscard]] int getNumberOfCheckpoints() const;

private:
  CheckpointFile() = default;

  std::unique_ptr<tarch::plotter::HDF5File> _file;
  int                                       _numberOfWriters{0};
  int                                       _numberOfCheckpoints{0};
};

#endif
