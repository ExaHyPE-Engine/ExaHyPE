// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/MigrationExchange.h"

#include "peano4/CellConnectivity.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
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
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

#include <mpi.h>

namespace {
  constexpr int Depth = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;

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
    peano4::CellConnectivity cellConnectivity{};
    const int*               cells{nullptr};
    Pool::View               chunks{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index) const {
      const int     slot  = cells[index];
      double* const chunk = chunks.getChunk(slot);
      chunk[0]            = cellConnectivity.getCellLevel(slot);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        chunk[1 + axis] = cellConnectivity.getCellLattice(slot)(axis);
      }
      chunk[MarkerEntry] = FilledMarker;
    }
  };

  /**
   * Writes the number of entries of one chunk that disagree with the cell of its slot or with a
   * migration marker.
   */
  struct CountMismatchesInSlot final {
    peano4::CellConnectivity cellConnectivity{};
    Pool::ConstView          chunks{};
    const int*               filledSlots{nullptr};
    int*                     mismatchesOfEntry{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      const int           slot       = filledSlots[entry];
      const double* const chunk      = chunks.getChunk(slot);
      int                 mismatches = 0;
      if (chunk[0] != cellConnectivity.getCellLevel(slot)) {
        mismatches++;
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (chunk[1 + axis] != cellConnectivity.getCellLattice(slot)(axis)) {
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
   * Writes the level and the lattice position of the child at childPosition of the cell parent into
   * target and marks it as prolonged.
   */
  GPU_CALLABLE_METHOD void writeChildOf(const double* const parent, const int childPosition, double* const target) {
    int position = childPosition;
    target[0]    = parent[0] + 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      target[1 + axis] = parent[1 + axis] * peano4::SubdivisionFactor + position % peano4::SubdivisionFactor;
      position /= peano4::SubdivisionFactor;
    }
    target[MarkerEntry] = ProlongedMarker;
  }

  /**
   * Writes the level and the lattice position of the parent of the cell firstChild into target and
   * marks it as restricted.
   */
  GPU_CALLABLE_METHOD void writeParentOf(const double* const firstChild, double* const target) {
    target[0] = firstChild[0] - 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int parentCoordinate = static_cast<int>(firstChild[1 + axis]) / peano4::SubdivisionFactor;
      target[1 + axis]           = static_cast<double>(parentCoordinate);
    }
    target[MarkerEntry] = RestrictedMarker;
  }

  /**
   * Derives the chunk of a new cell from the chunks of its old slots and marks how it arrived. One
   * work item fills the whole chunk.
   */
  struct IdentityTransfer final {
    static constexpr int WorkItemsPerChunk      = 1;
    static constexpr int PreparedValuesPerChunk = 0;

    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(
      const int                  slot,
      double* const              target,
      [[maybe_unused]] const int itemOfChunk
    ) const {
      const double* const kept = source.getChunk(slot);
      for (int entry = 0; entry < ChunkSize; entry++) {
        target[entry] = kept[entry];
      }
      target[MarkerEntry] = CopiedMarker;
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      const int                            parentSlot,
      double* const                        target,
      const int                            childPosition,
      [[maybe_unused]] const int           itemOfChunk,
      [[maybe_unused]] const double* const prepared
    ) const {
      writeChildOf(source.getChunk(parentSlot), childPosition, target);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      const int* const                     childSlots,
      double* const                        target,
      [[maybe_unused]] const int           itemOfChunk,
      [[maybe_unused]] const double* const prepared
    ) const {
      writeParentOf(source.getChunk(childSlots[0]), target);
    }
  };

  /**
   * An IdentityTransfer whose preparation item k copies the entry k of the parent or of the first
   * child into prepared. Its fills read the parent or the first child from prepared.
   */
  struct PreparedIdentityTransfer final {
    static constexpr int WorkItemsPerChunk        = 1;
    static constexpr int PreparedValuesPerChunk   = MarkerEntry;
    static constexpr int PreparationItemsPerChunk = MarkerEntry;

    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void prepareInterpolation(
      const int     parentSlot,
      double* const prepared,
      const int     itemOfChunk
    ) const {
      prepared[itemOfChunk] = source.getChunk(parentSlot)[itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void prepareRestriction(
      const int* const childSlots,
      double* const    prepared,
      const int        itemOfChunk
    ) const {
      prepared[itemOfChunk] = source.getChunk(childSlots[0])[itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(const int slot, double* const target, const int itemOfChunk) const {
      IdentityTransfer{source}.copy(slot, target, itemOfChunk);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      [[maybe_unused]] const int parentSlot,
      double* const              target,
      const int                  childPosition,
      [[maybe_unused]] const int itemOfChunk,
      const double* const        prepared
    ) const {
      writeChildOf(prepared, childPosition, target);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      [[maybe_unused]] const int* const childSlots,
      double* const                     target,
      [[maybe_unused]] const int        itemOfChunk,
      const double* const               prepared
    ) const {
      writeParentOf(prepared, target);
    }
  };
  TARGET_DECLARE_END

  /**
   * Writes the level and the lattice position of every cell into its chunk.
   */
  void fillChunksWithCellIdentity(Pool& pool, const peano4::CellConnectivity& cellConnectivity) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      FillChunkWithCellIdentity{
        cellConnectivity.onDevice(),
        cellConnectivity.onDevice().getCells().data,
        pool.getView()
      }
    );
  }

  int getNumberOfMismatchesInFilledSlots(
    const Pool&                     pool,
    const peano4::CellConnectivity& cellConnectivity,
    const std::vector<int>&         filledSlots
  ) {
    tarch::accelerator::Buffer<int> slots(pool.getLocation());
    slots.setValues(filledSlots);
    tarch::accelerator::Buffer<int> mismatchesOfEntry(filledSlots.size(), pool.getLocation());
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(static_cast<int>(filledSlots.size())),
      CountMismatchesInSlot{cellConnectivity.onDevice(), pool.getView(), slots.getData(), mismatchesOfEntry.getData()}
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
   * Runs one regrid with the requests, migrates the pool with a Transfer and returns the outcome.
   */
  template <typename Transfer>
  Outcome migrateAndCount(Exchange& exchange, Pool& pool, const std::vector<peano4::RefinementRequest>& requests) {
    auto&          spacetree        = peano4::Spacetree::getInstance();
    const auto&    cellConnectivity = spacetree.getCellConnectivity();
    peano4::Regrid regrid;
    if (not spacetree.regrid(requests, regrid)) {
      tarch::terminateWithError("The regrid of the test grid must change the grid.");
    }
    pool.reserve(cellConnectivity.getNumberOfSlots());

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

    exchange.migrate(pool, regrid, Transfer{std::as_const(pool).getView()});
    Outcome outcome;
    outcome.sentCells     = peano4::unittests::getGlobalSumOf(sentCells);
    outcome.receivedSlots = peano4::unittests::getGlobalSumOf(receivedSlots);
    outcome.locallyFilledSlots =
      peano4::unittests::getGlobalSumOf(static_cast<int>(filledSlots.size()) - receivedSlots);
    outcome.mismatches =
      peano4::unittests::getGlobalSumOf(getNumberOfMismatchesInFilledSlots(pool, cellConnectivity, filledSlots));
    spacetree.finishRegrid();
    fillChunksWithCellIdentity(pool, cellConnectivity);
    return outcome;
  }

  /**
   * Builds the regular grid with live data and writes the identity of every cell into its chunk.
   */
  void buildGridWithLiveChunks(Pool& pool) {
    peano4::unittests::buildRegularGrid(Depth);
    auto& spacetree = peano4::Spacetree::getInstance();
    spacetree.markPersistentDataLive();
    pool.reserve(spacetree.getCellConnectivity().getNumberOfSlots());
    fillChunksWithCellIdentity(pool, spacetree.getCellConnectivity());
  }
} // namespace

UNIT_TEST("A refinement and the erase back fill new slots from parents, children and other ranks with their cells") {
  GIVEN("A regular grid with live data, a pool with one chunk per cell that names the cell and an exchange") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    buildGridWithLiveChunks(pool);
    Exchange exchange(pool.getLocation());

    WHEN("The cell in the centre refines and the exchange migrates the chunks") {
      const Outcome refine =
        migrateAndCount<IdentityTransfer>(exchange, pool, {peano4::unittests::getRefinementOfCentreCell(Depth)});

      THEN("Every sent cell arrives, slots fill from parents and every filled slot stores the chunk of its cell") {
        VALIDATE_EQUALS(refine.sentCells, refine.receivedSlots);
        VALIDATE(refine.locallyFilledSlots > 0);
        VALIDATE_EQUALS(refine.mismatches, 0);
      }

      AND_WHEN("Every cell erases back and the exchange migrates the chunks") {
        const Outcome erase =
          migrateAndCount<IdentityTransfer>(exchange, pool, peano4::unittests::getErasureOfEveryCell(Depth));

        THEN("Every sent cell arrives, slots fill from children and every filled slot stores the chunk of its cell") {
          VALIDATE_EQUALS(erase.sentCells, erase.receivedSlots);
          VALIDATE(erase.locallyFilledSlots > 0);
          VALIDATE_EQUALS(erase.mismatches, 0);
        }
      }
    }
  }
}

UNIT_TEST("A preparing transfer fills every slot of a refinement and of the erase back with the chunk of its cell") {
  GIVEN("A regular grid with live data, a pool with one chunk per cell that names the cell and an exchange") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    buildGridWithLiveChunks(pool);
    Exchange exchange(pool.getLocation());

    WHEN("The cell in the centre refines and the exchange migrates the chunks with a preparing transfer") {
      const Outcome refine = migrateAndCount<PreparedIdentityTransfer>(
        exchange,
        pool,
        {peano4::unittests::getRefinementOfCentreCell(Depth)}
      );

      THEN("Every sent cell arrives, slots fill from parents and every filled slot stores the chunk of its cell") {
        VALIDATE_EQUALS(refine.sentCells, refine.receivedSlots);
        VALIDATE(refine.locallyFilledSlots > 0);
        VALIDATE_EQUALS(refine.mismatches, 0);
      }

      AND_WHEN("Every cell erases back and the exchange migrates the chunks with a preparing transfer") {
        const Outcome erase =
          migrateAndCount<PreparedIdentityTransfer>(exchange, pool, peano4::unittests::getErasureOfEveryCell(Depth));

        THEN("Every sent cell arrives, slots fill from children and every filled slot stores the chunk of its cell") {
          VALIDATE_EQUALS(erase.sentCells, erase.receivedSlots);
          VALIDATE(erase.locallyFilledSlots > 0);
          VALIDATE_EQUALS(erase.mismatches, 0);
        }
      }
    }
  }
}

UNIT_TEST("Two exchanges receive distinct tags in their first migration and every rank reserves the same tags") {
  GIVEN("A pool, two exchanges, a regrid without moves and the bytes the first exchange allocates") {
    Pool                 pool(ChunkSize, tarch::MemoryLocation::Device);
    Exchange             first(pool.getLocation());
    Exchange             second(pool.getLocation());
    const peano4::Regrid noMoves{};
    const std::size_t    bytesBeforeTheMigration = first.getAllocatedBytes().getTotal();

    WHEN("Both exchanges migrate the regrid and the ranks reduce both tags to their minimum and maximum") {
      first.migrate(pool, noMoves, IdentityTransfer{std::as_const(pool).getView()});
      second.migrate(pool, noMoves, IdentityTransfer{std::as_const(pool).getView()});

      const int      tags[2]{first.getTag(), second.getTag()};
      int            minimumTags[2]{0, 0};
      int            maximumTags[2]{0, 0};
      const MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
      tarch::mpi::allreduce(tags, minimumTags, 2, MPI_INT, MPI_MIN, communicator);
      tarch::mpi::allreduce(tags, maximumTags, 2, MPI_INT, MPI_MAX, communicator);

      THEN("The first exchange counted zero bytes, the second tag exceeds the first and the ranks agree on the tags") {
        VALIDATE_EQUALS(bytesBeforeTheMigration, std::size_t{0});
        VALIDATE(second.getTag() > first.getTag());
        VALIDATE_EQUALS(minimumTags[0], maximumTags[0]);
        VALIDATE_EQUALS(minimumTags[1], maximumTags[1]);
      }
    }
  }
}

UNIT_TEST("The release frees every buffer and keeps the tag and the next migration moves a later regrid") {
  GIVEN("A regular grid with live data, a pool with one chunk per cell that names the cell and an exchange") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    buildGridWithLiveChunks(pool);
    Exchange exchange(pool.getLocation());

    WHEN("The cell in the centre refines, the exchange migrates the chunks and then releases") {
      const Outcome refine =
        migrateAndCount<IdentityTransfer>(exchange, pool, {peano4::unittests::getRefinementOfCentreCell(Depth)});
      const int tag = exchange.getTag();
      exchange.release();
      const std::size_t bytesAfterTheRelease = exchange.getAllocatedBytes().getTotal();
      const int         tagAfterTheRelease   = exchange.getTag();

      THEN("Every filled slot stores the chunk of its cell and the exchange counts zero bytes and keeps its tag") {
        VALIDATE_EQUALS(refine.mismatches, 0);
        VALIDATE_EQUALS(bytesAfterTheRelease, std::size_t{0});
        VALIDATE_EQUALS(tagAfterTheRelease, tag);
      }

      AND_WHEN("Every cell erases back and the exchange migrates the chunks") {
        const Outcome erase =
          migrateAndCount<IdentityTransfer>(exchange, pool, peano4::unittests::getErasureOfEveryCell(Depth));

        THEN("Every sent cell arrives and every filled slot stores the chunk of its cell") {
          VALIDATE_EQUALS(erase.sentCells, erase.receivedSlots);
          VALIDATE_EQUALS(erase.mismatches, 0);
        }
      }
    }
  }
}
