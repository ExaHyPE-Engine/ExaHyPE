// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

#include "tarch/la/Vector.h"

/**
 * Reads and writes the grid group of a checkpoint file.
 *
 * The group holds what every rank reads: The domain, the base depth, the
 * refinement plan, the checkpoint time and one caller-defined integer table.
 * The per-solver cell data lives in the file's solver groups, which
 * CellDataWriter writes and CellDataLoader reads.
 */
namespace peano4::checkpoint {
  constexpr const char* GridGroup = "/grid";

  /** Creates the directories the file sits in; collective over every rank. */
  void createDirectoryFor(const std::string& path);

  /**
   * Writes the grid group of the checkpoint file from the refinement plan the
   * topology manager holds. extraTable travels as an integer table of
   * extraTableColumns columns under /grid/extra. Collective over every rank,
   * and every rank holds the checkpoint file open.
   */
  void writeGrid(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::vector<std::int32_t>&             extraTable,
    int                                          extraTableColumns,
    double                                       timeStamp
  );

  /**
   * Reads the grid group of the checkpoint file, verifies that dimensions and
   * domain match, and hands the stored refinement plan to the topology
   * manager, so the first grid sweep builds the checkpointed grid. Returns
   * the stored base depth.
   */
  int restoreGrid(
    const std::string&                           path,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize
  );

  /** Reads the /grid/extra table of the checkpoint file; columns receives its column count. */
  std::vector<std::int32_t> readExtraTable(const std::string& path, int& columns);
} // namespace peano4::checkpoint

#endif
