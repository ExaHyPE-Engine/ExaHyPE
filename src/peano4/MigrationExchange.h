// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

#include <mpi.h>

namespace peano4 {
  template <typename T>
  class MigrationExchange;
} // namespace peano4

/**
 * Fills the slots of the cells a regrid creates from the parent or the children on this rank, or
 * from the chunks another rank sends. The buffers use the memory location of the pool. With a GPU
 * backend MPI reads and writes device memory.
 */
template <typename T>
class peano4::MigrationExchange final {
public:
  using Pool = tarch::accelerator::ChunkedMemoryPool<T>;

  INLINE_METHOD explicit MigrationExchange(tarch::MemoryLocation location);
  MigrationExchange(const MigrationExchange& other)            = delete;
  MigrationExchange(MigrationExchange&& other)                 = delete;
  MigrationExchange& operator=(const MigrationExchange& other) = delete;
  MigrationExchange& operator=(MigrationExchange&& other)      = delete;
  ~MigrationExchange()                                         = default;

  template <typename Transfer>
  INLINE_METHOD void migrate(Pool& pool, const peano4::Regrid& regrid, const Transfer& transfer);
  INLINE_METHOD void release();

  [[nodiscard]] INLINE_METHOD int getTag() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;

private:
  template <typename Transfer>
  INLINE_METHOD void postTransfers(
    const Pool&                              pool,
    const std::vector<peano4::RankTransfer>& transfers,
    const Transfer&                          transfer,
    double*                                  prepared
  );
  INLINE_METHOD void receiveTransfers(Pool& pool);

  [[nodiscard]] static INLINE_METHOD int getNumberOfWorkItems(std::size_t chunks, int workItemsPerChunk);
  template <typename Transfer, typename Preparation>
  static INLINE_METHOD void prepareChunks(std::size_t chunks, const Preparation& preparation);

  static constexpr int NoTag = -1;

  int                      _tag{NoTag};
  std::vector<MPI_Request> _requests{};
  int                      _numberOfReceivedChunks{0};

  tarch::accelerator::Buffer<peano4::Prolongation>    _prolongations;
  tarch::accelerator::Buffer<peano4::Restriction>     _restrictions;
  tarch::accelerator::Buffer<peano4::TransferredCell> _sentCells;
  tarch::accelerator::Buffer<int>                     _receivedSlots;
  tarch::accelerator::Buffer<T>                       _sendBuffer;
  tarch::accelerator::Buffer<T>                       _receiveBuffer;
};

#include "peano4/MigrationExchange.cpph"
