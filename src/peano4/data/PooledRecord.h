// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

namespace peano4::data {
  template <class Record, typename DoF, int Cardinality2D, int Cardinality3D>
  class PooledRecord;
} // namespace peano4::data

/**
 * Stores the values of one grid entity in a chunk of the process-wide memory
 * pool of the derived type @p Record.
 *
 * A bound record references its pool chunk. An unbound record allocates and
 * owns storage on first access. The derived type adds the rank-boundary
 * protocol send(), receiveAndMerge() and merge().
 */
template <class Record, typename DoF, int Cardinality2D, int Cardinality3D>
class peano4::data::PooledRecord {
public:
  using DoFType = DoF;

  static constexpr int Cardinality = DIMENSIONS == 2 ? Cardinality2D : Cardinality3D;

  using MemoryPool = tarch::accelerator::ChunkedMemoryPool<DoFType, Cardinality>;

  PooledRecord() noexcept = default;
  PooledRecord(const PooledRecord& other);
  PooledRecord(PooledRecord&& other) noexcept;
  PooledRecord& operator=(const PooledRecord& other);
  PooledRecord& operator=(PooledRecord&& other) noexcept;
  ~PooledRecord() noexcept;

  static MemoryPool& getMemoryPool();

  static void bindPoolStorage(std::vector<Record>& slots);

  [[nodiscard]] INLINE_METHOD DoFType*       data();
  [[nodiscard]] INLINE_METHOD const DoFType* data() const noexcept;

  INLINE_METHOD void referenceValuesOf(const PooledRecord& other) noexcept;
  INLINE_METHOD void clearValues();

  template <class Marker>
  [[nodiscard]] INLINE_METHOD static bool load(const Marker& marker);

  static void startSend(peano4::grid::Stack<Record>& stack, peano4::grid::StackExchangeJob<Record>& job);
  static void startReceive(
    peano4::grid::Stack<Record>&            stack,
    peano4::grid::StackExchangeJob<Record>& job,
    int                                     numberOfElements
  );
  static bool tryToFinishSendOrReceive(peano4::grid::Stack<Record>& stack, peano4::grid::StackExchangeJob<Record>& job);

private:
  static constexpr std::size_t BytesPerRecord = static_cast<std::size_t>(Cardinality) * sizeof(DoFType);

  void releaseOwnStorage() noexcept;
  void adoptValuesOf(const PooledRecord& other);

  //! Points to a pool chunk or privately owned storage.
  DoFType* _data{nullptr};

  //! Records whether this record owns _data.
  bool _ownsStorage{false};

  //! Records whether _data belongs to a pool slot.
  bool _isPoolSlot{false};

  //! Records whether every value reads as zero. A record without storage
  //! counts as blank because data() clears its first allocation.
  bool _isBlank{true};
};

#include "peano4/data/PooledRecord.cpph"
