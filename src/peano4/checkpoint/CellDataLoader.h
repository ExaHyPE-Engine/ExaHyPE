// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/Lattice.h"
#include "tarch/la/Vector.h"
#include "tarch/reader/HDF5FileReader.h"

namespace peano4::checkpoint {
  class CellDataLoader;
} // namespace peano4::checkpoint

/**
 * Reads one solver's cell records back from a checkpoint file.
 *
 * loadLocalRecords() reads, per level, every block whose subtree holds cells
 * of this rank, so the resident data is bounded by the cells this rank owns
 * plus the blocks it borders. It runs single-threaded before the mesh
 * traversal; record() answers the traversal's per-cell queries from the
 * resident tables without a lock.
 */
class peano4::checkpoint::CellDataLoader {
public:
  CellDataLoader(const std::string& path, int valuesPerRecord);
  CellDataLoader(const CellDataLoader&)            = delete;
  CellDataLoader(CellDataLoader&&)                 = delete;
  CellDataLoader& operator=(const CellDataLoader&) = delete;
  CellDataLoader& operator=(CellDataLoader&&)      = delete;
  ~CellDataLoader() = default;

  void loadLocalRecords();

  /** Returns the record's values, or nullptr when the checkpoint holds no record for the cell. */
  [[nodiscard]] const double* record(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;

  [[nodiscard]] double getTimeStamp() const;

  [[nodiscard]] const std::vector<double>& getSolverScalars() const;

private:
  tarch::reader::HDF5FileReader _reader;
  const int                     _valuesPerRecord;

  double              _timeStamp{0.0};
  std::vector<double> _solverScalars;

  int _depth{0};

  /** Per level (counted from _depth): The record values of the resident blocks. */
  std::vector<std::vector<double>> _values;

  /** Per level (counted from _depth): Each resident key's index into the level's values. */
  std::vector<std::unordered_map<std::int64_t, std::size_t>> _recordOfKey;
};

#endif
