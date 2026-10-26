// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/communication/MigrationExchangeKernels.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.h"

namespace {
  TARGET_DECLARE
  constexpr int NumberOfSlots = 2 * peano4::ChildrenPerCell;
  constexpr int ChunkSize     = 2;
  TARGET_DECLARE_END

  using Pool = tarch::accelerator::ChunkedMemoryPool<double>;

  /**
   * Returns the value the test writes into element of the slot.
   */
  GPU_CALLABLE_METHOD double getValueOf(const int slot, const int element) { return 10.0 * slot + element; }

  TARGET_DECLARE
  /**
   * A transfer that copies an element, adds the child position to the element of the parent and
   * writes the sum of the element of the children. Every element is one work item.
   */
  struct SumTransfer final {
    static constexpr int WorkItemsPerChunk      = ChunkSize;
    static constexpr int PreparedValuesPerChunk = 0;

    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(const int slot, double* const target, const int itemOfChunk) const {
      target[itemOfChunk] = source.getChunk(slot)[itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      const int                            parentSlot,
      double* const                        target,
      const int                            childPosition,
      const int                            itemOfChunk,
      [[maybe_unused]] const double* const prepared
    ) const {
      target[itemOfChunk] = source.getChunk(parentSlot)[itemOfChunk] + childPosition;
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      const int* const                     childSlots,
      double* const                        target,
      const int                            itemOfChunk,
      [[maybe_unused]] const double* const prepared
    ) const {
      target[itemOfChunk] = 0.0;
      for (int child = 0; child < peano4::ChildrenPerCell; child++) {
        target[itemOfChunk] += source.getChunk(childSlots[child])[itemOfChunk];
      }
    }
  };

  /**
   * A SumTransfer that prepares the elements of the parent or of the first child. Preparation item
   * k copies element k into prepared[k + 1]. Work item w adds prepared[ChunkSize - w].
   */
  struct PreparedSumTransfer final {
    static constexpr int WorkItemsPerChunk        = ChunkSize;
    static constexpr int PreparedValuesPerChunk   = ChunkSize + 1;
    static constexpr int PreparationItemsPerChunk = ChunkSize;

    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void prepareInterpolation(
      const int     parentSlot,
      double* const prepared,
      const int     itemOfChunk
    ) const {
      prepared[itemOfChunk + 1] = source.getChunk(parentSlot)[itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void prepareRestriction(
      const int* const childSlots,
      double* const    prepared,
      const int        itemOfChunk
    ) const {
      prepared[itemOfChunk + 1] = source.getChunk(childSlots[0])[itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(const int slot, double* const target, const int itemOfChunk) const {
      SumTransfer{source}.copy(slot, target, itemOfChunk);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      const int           parentSlot,
      double* const       target,
      const int           childPosition,
      const int           itemOfChunk,
      const double* const prepared
    ) const {
      SumTransfer{source}.interpolateFromParent(parentSlot, target, childPosition, itemOfChunk, nullptr);
      target[itemOfChunk] += prepared[ChunkSize - itemOfChunk];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      const int* const    childSlots,
      double* const       target,
      const int           itemOfChunk,
      const double* const prepared
    ) const {
      SumTransfer{source}.restrictFromChildren(childSlots, target, itemOfChunk, nullptr);
      target[itemOfChunk] += prepared[ChunkSize - itemOfChunk];
    }
  };
  TARGET_DECLARE_END

  Pool getFilledPool() {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    pool.reserve(NumberOfSlots);
    std::vector<double> chunks          = tarch::accelerator::unittests::getChunksOf(pool);
    const auto          paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());
    for (int slot = 0; slot < NumberOfSlots; slot++) {
      for (int element = 0; element < ChunkSize; element++) {
        chunks[static_cast<std::size_t>(slot) * paddedChunkSize + static_cast<std::size_t>(element)] =
          getValueOf(slot, element);
      }
    }
    tarch::accelerator::unittests::setChunksOf(pool, chunks);
    return pool;
  }

  double getChunkValue(const Pool& pool, const int slot, const int element) {
    const std::vector<double> chunks = tarch::accelerator::unittests::getChunksOf(pool);
    return chunks
      [static_cast<std::size_t>(slot) * static_cast<std::size_t>(pool.getPaddedChunkSize())
       + static_cast<std::size_t>(element)];
  }

  /**
   * Returns the sum of getValueOf() over the first ChildrenPerCell slots.
   */
  double getSumOfFirstChildren(const int element) {
    double sum = 0.0;
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      sum += getValueOf(child, element);
    }
    return sum;
  }

  peano4::Restriction getRestrictionOfFirstChildren(const int parentSlot) {
    peano4::Restriction restriction;
    restriction.parentSlot = parentSlot;
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      restriction.childSlots[child] = child;
    }
    return restriction;
  }
} // namespace

UNIT_TEST("The prolongation fills the slot of a child from the chunk of its parent and the position of the child") {
  GIVEN("A filled pool and the prolongation from slot zero into the last slot at the child position three") {
    Pool                                             pool = getFilledPool();
    tarch::accelerator::Buffer<peano4::Prolongation> prolongations(tarch::MemoryLocation::Device);
    prolongations.setValues(std::vector<peano4::Prolongation>{{0, NumberOfSlots - 1, 3}});

    WHEN("The prolongation kernel runs with a transfer that adds the child position to the chunk of the parent") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(SumTransfer::WorkItemsPerChunk),
        peano4::communication::ProlongChunks<double, SumTransfer>{
          prolongations.getData(),
          pool.getView(),
          SumTransfer{std::as_const(pool).getView()}
        }
      );

      THEN("The kernel writes the values of slot zero plus three into the last slot") {
        for (int element = 0; element < ChunkSize; element++) {
          VALIDATE_NUMERICAL_EQUALS(
            getChunkValue(pool, NumberOfSlots - 1, element),
            getValueOf(0, element) + 3.0,
            element
          );
        }
      }
    }
  }
}

UNIT_TEST("The restriction fills the slot of a parent from the chunks of its children") {
  GIVEN("A filled pool and the restriction from the first slots as children into the last slot") {
    Pool                                            pool = getFilledPool();
    tarch::accelerator::Buffer<peano4::Restriction> restrictions(tarch::MemoryLocation::Device);
    restrictions.setValues(std::vector<peano4::Restriction>{getRestrictionOfFirstChildren(NumberOfSlots - 1)});

    WHEN("The restriction kernel runs with a transfer that sums the chunks of the children") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(SumTransfer::WorkItemsPerChunk),
        peano4::communication::RestrictChunks<double, SumTransfer>{
          restrictions.getData(),
          pool.getView(),
          SumTransfer{std::as_const(pool).getView()}
        }
      );

      THEN("The kernel writes the sum of the values of the children into the last slot") {
        for (int element = 0; element < ChunkSize; element++) {
          VALIDATE_NUMERICAL_EQUALS(
            getChunkValue(pool, NumberOfSlots - 1, element),
            getSumOfFirstChildren(element),
            element
          );
        }
      }
    }
  }
}

UNIT_TEST("The gather takes every kind of sent cell into the send buffer in the order of the cells") {
  GIVEN("A filled pool, a send buffer and one kept, one interpolated and one restricted cell") {
    const Pool              pool = getFilledPool();
    peano4::TransferredCell kept;
    kept.keptSlot = 1;
    peano4::TransferredCell fromParent;
    fromParent.parentSlot    = 0;
    fromParent.childPosition = 2;
    peano4::TransferredCell fromChildren;
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      fromChildren.childSlots[child] = child;
    }
    tarch::accelerator::Buffer<peano4::TransferredCell> cells(tarch::MemoryLocation::Device);
    cells.setValues(std::vector<peano4::TransferredCell>{kept, fromParent, fromChildren});
    tarch::accelerator::Buffer<double> send(3 * ChunkSize, tarch::MemoryLocation::Device);

    WHEN("The gather kernel runs over the three cells") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(3 * SumTransfer::WorkItemsPerChunk),
        peano4::communication::GatherChunks<double, SumTransfer>{
          cells.getData(),
          send.getData(),
          SumTransfer{pool.getView()},
          ChunkSize
        }
      );
      const std::vector<double> sent = send.getValues();

      THEN("The gather writes the kept chunk, the interpolated child and the restricted parent in this order") {
        for (int element = 0; element < ChunkSize; element++) {
          const auto index = static_cast<std::size_t>(element);
          VALIDATE_NUMERICAL_EQUALS(sent[index], getValueOf(1, element), element);
          VALIDATE_NUMERICAL_EQUALS(sent[ChunkSize + index], getValueOf(0, element) + 2.0, element);
          VALIDATE_NUMERICAL_EQUALS(sent[2 * ChunkSize + index], getSumOfFirstChildren(element), element);
        }
      }
    }
  }
}

UNIT_TEST(
  "The prolongation, the restriction and the gather of a preparing transfer read what every preparation item wrote"
) {
  GIVEN("A filled pool, a preparing transfer, a prolongation, a restriction, three sent cells and a prepared buffer") {
    constexpr int Values = PreparedSumTransfer::PreparedValuesPerChunk;
    // The blocks of prepared belong to the prolongation, the restriction, kept, fromParent and
    // fromChildren in this order. Slot -1 marks the block of kept that the preparation skips.
    constexpr std::array<int, 5>                     SlotOfBlock{1, 0, -1, 2, 0};
    Pool                                             pool = getFilledPool();
    const PreparedSumTransfer                        transfer{std::as_const(pool).getView()};
    tarch::accelerator::Buffer<peano4::Prolongation> prolongations(tarch::MemoryLocation::Device);
    prolongations.setValues(std::vector<peano4::Prolongation>{{1, NumberOfSlots - 1, 3}});
    tarch::accelerator::Buffer<peano4::Restriction> restrictions(tarch::MemoryLocation::Device);
    restrictions.setValues(std::vector<peano4::Restriction>{getRestrictionOfFirstChildren(NumberOfSlots - 2)});
    peano4::TransferredCell kept;
    kept.keptSlot = 1;
    peano4::TransferredCell fromParent;
    fromParent.parentSlot    = 2;
    fromParent.childPosition = 2;
    peano4::TransferredCell fromChildren;
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      fromChildren.childSlots[child] = child;
    }
    tarch::accelerator::Buffer<peano4::TransferredCell> cells(tarch::MemoryLocation::Device);
    cells.setValues(std::vector<peano4::TransferredCell>{kept, fromParent, fromChildren});
    tarch::accelerator::Buffer<double> send(3 * ChunkSize, tarch::MemoryLocation::Device);
    tarch::accelerator::Buffer<double> prepared(tarch::MemoryLocation::Device);
    prepared.setValues(std::vector<double>(SlotOfBlock.size() * Values, -1.0));

    WHEN("The preparation kernels run and then the prolongation, the restriction and the gather kernels") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(PreparedSumTransfer::PreparationItemsPerChunk),
        peano4::communication::PrepareProlongations<PreparedSumTransfer>{
          prolongations.getData(),
          transfer,
          prepared.getData()
        }
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(PreparedSumTransfer::PreparationItemsPerChunk),
        peano4::communication::PrepareRestrictions<PreparedSumTransfer>{
          restrictions.getData(),
          transfer,
          prepared.getData() + Values
        }
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(3 * PreparedSumTransfer::PreparationItemsPerChunk),
        peano4::communication::PrepareSentCells<PreparedSumTransfer>{
          cells.getData(),
          transfer,
          prepared.getData() + 2 * Values
        }
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(PreparedSumTransfer::WorkItemsPerChunk),
        peano4::communication::ProlongChunks<double, PreparedSumTransfer>{
          prolongations.getData(),
          pool.getView(),
          transfer,
          prepared.getData()
        }
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(PreparedSumTransfer::WorkItemsPerChunk),
        peano4::communication::RestrictChunks<double, PreparedSumTransfer>{
          restrictions.getData(),
          pool.getView(),
          transfer,
          prepared.getData() + Values
        }
      );
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(3 * PreparedSumTransfer::WorkItemsPerChunk),
        peano4::communication::GatherChunks<double, PreparedSumTransfer>{
          cells.getData(),
          send.getData(),
          transfer,
          ChunkSize,
          prepared.getData() + 2 * Values
        }
      );
      const std::vector<double> preparedValues = prepared.getValues();
      const std::vector<double> sent           = send.getValues();

      THEN("The preparation copies the slot of every block behind its first value and leaves the kept block at -1") {
        for (std::size_t block = 0; block < SlotOfBlock.size(); block++) {
          const std::size_t first = block * Values;
          VALIDATE_NUMERICAL_EQUALS(preparedValues[first], -1.0, block);
          for (int element = 0; element < ChunkSize; element++) {
            VALIDATE_NUMERICAL_EQUALS(
              preparedValues[first + 1 + static_cast<std::size_t>(element)],
              SlotOfBlock[block] < 0 ? -1.0 : getValueOf(SlotOfBlock[block], element),
              block,
              element
            );
          }
        }
      }

      THEN("Every interpolated or restricted chunk adds the mirrored prepared element and the kept chunk is a copy") {
        for (int element = 0; element < ChunkSize; element++) {
          const auto index    = static_cast<std::size_t>(element);
          const int  mirrored = ChunkSize - 1 - element;
          VALIDATE_NUMERICAL_EQUALS(
            getChunkValue(pool, NumberOfSlots - 1, element),
            getValueOf(1, element) + 3.0 + getValueOf(1, mirrored),
            element
          );
          VALIDATE_NUMERICAL_EQUALS(
            getChunkValue(pool, NumberOfSlots - 2, element),
            getSumOfFirstChildren(element) + getValueOf(0, mirrored),
            element
          );
          VALIDATE_NUMERICAL_EQUALS(sent[index], getValueOf(1, element), element);
          VALIDATE_NUMERICAL_EQUALS(
            sent[ChunkSize + index],
            getValueOf(2, element) + 2.0 + getValueOf(2, mirrored),
            element
          );
          VALIDATE_NUMERICAL_EQUALS(
            sent[2 * ChunkSize + index],
            getSumOfFirstChildren(element) + getValueOf(0, mirrored),
            element
          );
        }
      }
    }
  }
}

UNIT_TEST("The scatter writes every received chunk into its slot and leaves the other slots unchanged") {
  GIVEN("A filled pool, the receiving slots three and one and the two received chunks") {
    Pool                            pool = getFilledPool();
    tarch::accelerator::Buffer<int> slots(tarch::MemoryLocation::Device);
    slots.setValues(std::vector<int>{3, 1});
    tarch::accelerator::Buffer<double> received(tarch::MemoryLocation::Device);
    received.setValues(std::vector<double>{100.0, 101.0, 200.0, 201.0});

    WHEN("The scatter kernel runs over the received chunks") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(2 * ChunkSize),
        peano4::communication::ScatterChunks<double>{slots.getData(), received.getData(), pool.getView(), ChunkSize}
      );

      THEN("The scatter writes the first chunk into slot three, the second into slot one and keeps slot zero") {
        for (int element = 0; element < ChunkSize; element++) {
          VALIDATE_NUMERICAL_EQUALS(getChunkValue(pool, 3, element), 100.0 + element, element);
          VALIDATE_NUMERICAL_EQUALS(getChunkValue(pool, 1, element), 200.0 + element, element);
          VALIDATE_NUMERICAL_EQUALS(getChunkValue(pool, 0, element), getValueOf(0, element), element);
        }
      }
    }
  }
}
