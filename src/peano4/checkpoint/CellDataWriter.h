// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/Lattice.h"
#include "tarch/la/Vector.h"

namespace peano4::checkpoint {
  class CellDataWriter;
} // namespace peano4::checkpoint

/**
 * Writes one solver's cell records into one group of the checkpoint file.
 *
 * Every mesh traversal task appends the records of the leaf cells it visits
 * into its own Records buffer and hands the buffer over through collect().
 * write() then derives each record's global row from the refinement plan,
 * which is identical on every rank, and places every rank's rows into the
 * group with one collective write per dataset. The group holds, per level,
 * the per-block record counts, the sorted triadic Morton keys and the record
 * values, plus the solver's scalar state and the checkpoint time.
 */
class peano4::checkpoint::CellDataWriter final {
public:
  /** One traversal task's record buffer; append() runs without synchronisation. */
  struct Records {
    std::vector<std::int32_t> levels;
    std::vector<std::int64_t> keys;
    std::vector<double>       values;

    template <typename SourceType>
    INLINE_METHOD void append(
      int                                       level,
      const tarch::la::Vector<DIMENSIONS, int>& latticeIndex,
      const SourceType*                         cellValues,
      int                                       count
    );
    void clear();
  };

  /** The group is the checkpoint file's group this solver writes into, such as "/solvers/Euler". */
  CellDataWriter(const std::string& group, int valuesPerRecord);
  CellDataWriter(const CellDataWriter&)            = delete;
  CellDataWriter(CellDataWriter&&)                 = delete;
  CellDataWriter& operator=(const CellDataWriter&) = delete;
  CellDataWriter& operator=(CellDataWriter&&)      = delete;
  ~CellDataWriter()                                = default;

  void collect(Records&& records);

  /**
   * Writes the collected records, the solver scalars and the time stamp.
   * Collective over every rank.
   */
  void write(const std::vector<double>& solverScalars, double timeStamp);

private:
  const std::string _group;
  const int         _valuesPerRecord;

  /**
   * Each traversal task takes this lock once, when collect() hands its buffer
   * over. The buffer moves into the deque, which no atomic operation performs.
   */
  std::mutex          _collectMutex;
  std::deque<Records> _collectedRecords;
};

#include "peano4/checkpoint/CellDataWriter.cpph"

#endif
