// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MigrationExchangeTest.h"

#include "peano4/MigrationExchange.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  constexpr int Depth = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;

  TARGET_DECLARE
  constexpr int ChunkSize   = DIMENSIONS + 2;
  constexpr int MarkerEntry = DIMENSIONS + 1;

  /**
   * The marker entry records how a chunk arrived: Filled, copied, prolonged or restricted.
   */
  constexpr double FilledMarker     = 1.0;
  constexpr double CopiedMarker     = 2.0;
  constexpr double ProlongedMarker  = 3.0;
  constexpr double RestrictedMarker = 4.0;
  TARGET_DECLARE_END

  using Pool     = tarch::accelerator::ChunkedMemoryPool<double>;
  using Exchange = peano4::MigrationExchange<double>;

  TARGET_DECLARE
  /**
   * Writes the level and the lattice position of one cell into its chunk.
   */
  struct FillChunkWithCellIdentity final {
    peano4::Partition partition{};
    const int*        cells{nullptr};
    Pool::View        chunks{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index) const {
      const int     slot  = cells[index];
      double* const chunk = chunks.getChunk(slot);
      chunk[0]            = partition.getCellLevel(slot);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        chunk[1 + axis] = partition.getCellLattice(slot)(axis);
      }
      chunk[MarkerEntry] = FilledMarker;
    }
  };

  /**
   * Writes the number of entries of one chunk that disagree with the cell of its slot or with a
   * migration marker.
   */
  struct CountMismatchesInSlot final {
    peano4::Partition partition{};
    Pool::ConstView   chunks{};
    const int*        filledSlots{nullptr};
    int*              mismatchesOfEntry{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      const int           slot       = filledSlots[entry];
      const double* const chunk      = chunks.getChunk(slot);
      int                 mismatches = 0;
      if (chunk[0] != partition.getCellLevel(slot)) {
        mismatches++;
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (chunk[1 + axis] != partition.getCellLattice(slot)(axis)) {
          mismatches++;
        }
      }
      if (
        chunk[MarkerEntry] != CopiedMarker and chunk[MarkerEntry] != ProlongedMarker
        and chunk[MarkerEntry] != RestrictedMarker
      ) {
        mismatches++;
      }
      mismatchesOfEntry[entry] = mismatches;
    }
  };

  /**
   * Derives the chunk of a new cell from the chunks of its old slots and marks how it arrived.
   */
  struct IdentityTransfer final {
    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(const int slot, double* const target) const {
      const double* const kept = source.getChunk(slot);
      for (int entry = 0; entry < ChunkSize; entry++) {
        target[entry] = kept[entry];
      }
      target[MarkerEntry] = CopiedMarker;
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      const int     parentSlot,
      double* const target,
      const int     childPosition
    ) const {
      const double* const parent   = source.getChunk(parentSlot);
      int                 position = childPosition;
      target[0]                    = parent[0] + 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        target[1 + axis] = parent[1 + axis] * peano4::SubdivisionFactor + position % peano4::SubdivisionFactor;
        position /= peano4::SubdivisionFactor;
      }
      target[MarkerEntry] = ProlongedMarker;
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      const int* const childSlots,
      double* const    target
    ) const {
      const double* const firstChild = source.getChunk(childSlots[0]);
      target[0]                      = firstChild[0] - 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const int parentCoordinate = static_cast<int>(firstChild[1 + axis]) / peano4::SubdivisionFactor;
        target[1 + axis]           = static_cast<double>(parentCoordinate);
      }
      target[MarkerEntry] = RestrictedMarker;
    }
  };
  TARGET_DECLARE_END

  /**
   * Writes the level and the lattice position of every cell into its chunk.
   */
  void fillChunksWithCellIdentity(Pool& pool, const peano4::Partition& partition) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(partition.getCells().size),
      FillChunkWithCellIdentity{partition.onDevice(), partition.onDevice().getCells().data, pool.getView()}
    );
  }

  int getNumberOfMismatchesInFilledSlots(
    const Pool&              pool,
    const peano4::Partition& partition,
    const std::vector<int>&  filledSlots
  ) {
    tarch::accelerator::Buffer<int> slots(pool.getLocation());
    slots.setValues(filledSlots);
    tarch::accelerator::Buffer<int> mismatchesOfEntry(filledSlots.size(), pool.getLocation());
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(static_cast<int>(filledSlots.size())),
      CountMismatchesInSlot{partition.onDevice(), pool.getView(), slots.getData(), mismatchesOfEntry.getData()}
    );
    const std::vector<int> mismatches = mismatchesOfEntry.getValues();
    return std::accumulate(mismatches.begin(), mismatches.end(), 0);
  }

  /**
   * The moves of one regrid, summed over all ranks.
   */
  struct Outcome final {
    int sentCells{0};
    int receivedSlots{0};
    int locallyFilledSlots{0};
    int mismatches{0};
  };

  /**
   * Runs one regrid with the requests, migrates the pool and returns the outcome.
   */
  Outcome migrateAndCount(Exchange& exchange, Pool& pool, const std::vector<peano4::RefinementRequest>& requests) {
    auto&          spacetree = peano4::Spacetree::getInstance();
    const auto&    partition = spacetree.getPartition();
    peano4::Regrid regrid;
    if (not spacetree.regrid(requests, regrid)) {
      tarch::terminateWithError("The regrid of the test grid changed nothing.");
    }
    pool.reserve(partition.getNumberOfSlots());

    std::vector<int> filledSlots;
    int              sentCells = 0;
    for (const auto& rankTransfer : regrid.rankTransfers) {
      filledSlots.insert(filledSlots.end(), rankTransfer.receivedSlots.begin(), rankTransfer.receivedSlots.end());
      sentCells += static_cast<int>(rankTransfer.sentCells.size());
    }
    const int receivedSlots = static_cast<int>(filledSlots.size());
    for (const auto& prolongation : regrid.prolongations) {
      filledSlots.push_back(prolongation.childSlot);
    }
    for (const auto& restriction : regrid.restrictions) {
      filledSlots.push_back(restriction.parentSlot);
    }

    exchange.migrate(pool, regrid, IdentityTransfer{std::as_const(pool).getView()});
    Outcome outcome;
    outcome.sentCells          = peano4::tests::getGlobalSumOf(sentCells);
    outcome.receivedSlots      = peano4::tests::getGlobalSumOf(receivedSlots);
    outcome.locallyFilledSlots = peano4::tests::getGlobalSumOf(static_cast<int>(filledSlots.size()) - receivedSlots);
    outcome.mismatches =
      peano4::tests::getGlobalSumOf(getNumberOfMismatchesInFilledSlots(pool, partition, filledSlots));
    spacetree.finishRegrid();
    fillChunksWithCellIdentity(pool, partition);
    return outcome;
  }

  /**
   * Builds the regular grid with live data and writes the identity of every cell into its chunk.
   */
  void buildGridWithLiveChunks(Pool& pool) {
    peano4::tests::buildRegularGrid(Depth);
    auto& spacetree = peano4::Spacetree::getInstance();
    spacetree.markPersistentDataLive();
    pool.reserve(spacetree.getPartition().getNumberOfSlots());
    fillChunksWithCellIdentity(pool, spacetree.getPartition());
  }
} // namespace

peano4::tests::MigrationExchangeTest::MigrationExchangeTest() :
  tarch::tests::TestCase("peano4::tests::MigrationExchangeTest") {}

void
peano4::tests::MigrationExchangeTest::run() {
  testMethod(testEveryFilledSlotHoldsTheChunkOfItsCell);
  testMethod(testExchangesHoldDistinctTags);
  testMethod(testReleaseFreesTheBuffersAndKeepsTheTag);
}

/**
 * After a refinement and the erase back, every filled slot stores the chunk of its cell and every
 * sent cell arrives.
 */
void
peano4::tests::MigrationExchangeTest::testEveryFilledSlotHoldsTheChunkOfItsCell() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Device);
  buildGridWithLiveChunks(pool);
  Exchange exchange(pool.getLocation());

  const Outcome refine = migrateAndCount(exchange, pool, {peano4::tests::getRefinementOfCentreCell(Depth)});
  validateEquals(refine.sentCells, refine.receivedSlots);
  validate(refine.locallyFilledSlots > 0);
  validateEquals(refine.mismatches, 0);

  const Outcome erase = migrateAndCount(exchange, pool, peano4::tests::getErasureOfEveryCell(Depth));
  validateEquals(erase.sentCells, erase.receivedSlots);
  validate(erase.locallyFilledSlots > 0);
  validateEquals(erase.mismatches, 0);
}

/**
 * Two exchanges get distinct tags in their first migrate, and every rank reserves the same tags.
 */
void
peano4::tests::MigrationExchangeTest::testExchangesHoldDistinctTags() {
  Pool                 pool(ChunkSize, tarch::MemoryLocation::Device);
  Exchange             first(pool.getLocation());
  Exchange             second(pool.getLocation());
  const peano4::Regrid noMoves{};
  validateEquals(first.getAllocatedBytes().getTotal(), std::size_t{0});
  first.migrate(pool, noMoves, IdentityTransfer{std::as_const(pool).getView()});
  second.migrate(pool, noMoves, IdentityTransfer{std::as_const(pool).getView()});
  validate(second.getTag() > first.getTag());

  const int tags[2]{first.getTag(), second.getTag()};
  int       minimumTags[2]{0, 0};
  int       maximumTags[2]{0, 0};
  MPI_Comm  communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(tags, minimumTags, 2, MPI_INT, MPI_MIN, communicator);
  tarch::mpi::allreduce(tags, maximumTags, 2, MPI_INT, MPI_MAX, communicator);
  validateEquals(minimumTags[0], maximumTags[0]);
  validateEquals(minimumTags[1], maximumTags[1]);
}

/**
 * The release frees every buffer and keeps the tag, and the next migrate moves the chunks of a later
 * regrid correctly.
 */
void
peano4::tests::MigrationExchangeTest::testReleaseFreesTheBuffersAndKeepsTheTag() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Device);
  buildGridWithLiveChunks(pool);
  Exchange exchange(pool.getLocation());

  const Outcome refine = migrateAndCount(exchange, pool, {peano4::tests::getRefinementOfCentreCell(Depth)});
  validateEquals(refine.mismatches, 0);
  const int tag = exchange.getTag();

  exchange.release();
  validateEquals(exchange.getAllocatedBytes().getTotal(), std::size_t{0});
  validateEquals(exchange.getTag(), tag);

  const Outcome erase = migrateAndCount(exchange, pool, peano4::tests::getErasureOfEveryCell(Depth));
  validateEquals(erase.sentCells, erase.receivedSlots);
  validateEquals(erase.mismatches, 0);
}
