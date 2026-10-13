// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MigrationExchangeKernelsTest.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Regrid.h"
#include "peano4/communication/MigrationExchangeKernels.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

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
   * A transfer that copies a chunk, adds the child position to the parent's values and writes
   * the sum of the children's values.
   */
  struct SumTransfer final {
    Pool::ConstView source{};

    INLINE_METHOD GPU_CALLABLE_METHOD void copy(const int slot, double* const target) const {
      for (int element = 0; element < ChunkSize; element++) {
        target[element] = source.getChunk(slot)[element];
      }
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
      const int     parentSlot,
      double* const target,
      const int     childPosition
    ) const {
      for (int element = 0; element < ChunkSize; element++) {
        target[element] = source.getChunk(parentSlot)[element] + childPosition;
      }
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
      const int* const childSlots,
      double* const    target
    ) const {
      for (int element = 0; element < ChunkSize; element++) {
        target[element] = 0.0;
        for (int child = 0; child < peano4::ChildrenPerCell; child++) {
          target[element] += source.getChunk(childSlots[child])[element];
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Fills every chunk of a device pool with getValueOf() and returns the pool.
   */
  Pool getFilledPool() {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    pool.reserve(NumberOfSlots);
    std::vector<double> chunks;
    pool.copyToHost(chunks);
    const auto paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());
    for (int slot = 0; slot < NumberOfSlots; slot++) {
      for (int element = 0; element < ChunkSize; element++) {
        chunks[static_cast<std::size_t>(slot) * paddedChunkSize + static_cast<std::size_t>(element)] =
          getValueOf(slot, element);
      }
    }
    pool.copyFromHost(chunks);
    return pool;
  }

  double getChunkValue(const Pool& pool, const int slot, const int element) {
    std::vector<double> chunks;
    pool.copyToHost(chunks);
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

peano4::communication::tests::MigrationExchangeKernelsTest::MigrationExchangeKernelsTest() :
  tarch::tests::TestCase("peano4::communication::tests::MigrationExchangeKernelsTest") {}

void
peano4::communication::tests::MigrationExchangeKernelsTest::run() {
  testMethod(testProlongFillsTheChildFromItsParent);
  testMethod(testRestrictFillsTheParentFromItsChildren);
  testMethod(testGatherTakesEveryKindOfSentCell);
  testMethod(testScatterWritesTheReceivedChunks);
}

void
peano4::communication::tests::MigrationExchangeKernelsTest::testProlongFillsTheChildFromItsParent() {
  Pool                                             pool = getFilledPool();
  tarch::accelerator::Buffer<peano4::Prolongation> prolongations(tarch::MemoryLocation::Device);
  prolongations.setValues(std::vector<peano4::Prolongation>{{0, NumberOfSlots - 1, 3}});

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(1),
    peano4::communication::ProlongChunks<double, SumTransfer>{
      prolongations.getData(),
      pool.getView(),
      SumTransfer{std::as_const(pool).getView()}
    }
  );

  for (int element = 0; element < ChunkSize; element++) {
    validateNumericalEqualsWithParams1(
      getChunkValue(pool, NumberOfSlots - 1, element),
      getValueOf(0, element) + 3.0,
      element
    );
  }
}

void
peano4::communication::tests::MigrationExchangeKernelsTest::testRestrictFillsTheParentFromItsChildren() {
  Pool                                            pool = getFilledPool();
  tarch::accelerator::Buffer<peano4::Restriction> restrictions(tarch::MemoryLocation::Device);
  restrictions.setValues(std::vector<peano4::Restriction>{getRestrictionOfFirstChildren(NumberOfSlots - 1)});

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(1),
    peano4::communication::RestrictChunks<double, SumTransfer>{
      restrictions.getData(),
      pool.getView(),
      SumTransfer{std::as_const(pool).getView()}
    }
  );

  for (int element = 0; element < ChunkSize; element++) {
    validateNumericalEqualsWithParams1(
      getChunkValue(pool, NumberOfSlots - 1, element),
      getSumOfFirstChildren(element),
      element
    );
  }
}

/**
 * The gather writes the kept cell, the interpolated child and the restricted parent into the send
 * buffer in the order of the cells.
 */
void
peano4::communication::tests::MigrationExchangeKernelsTest::testGatherTakesEveryKindOfSentCell() {
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

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(3),
    peano4::communication::GatherChunks<double, SumTransfer>{
      cells.getData(),
      send.getData(),
      SumTransfer{pool.getView()},
      ChunkSize
    }
  );

  const std::vector<double> sent = send.getValues();
  for (int element = 0; element < ChunkSize; element++) {
    const auto index = static_cast<std::size_t>(element);
    validateNumericalEqualsWithParams1(sent[index], getValueOf(1, element), element);
    validateNumericalEqualsWithParams1(sent[ChunkSize + index], getValueOf(0, element) + 2.0, element);
    validateNumericalEqualsWithParams1(sent[2 * ChunkSize + index], getSumOfFirstChildren(element), element);
  }
}

void
peano4::communication::tests::MigrationExchangeKernelsTest::testScatterWritesTheReceivedChunks() {
  Pool                            pool = getFilledPool();
  tarch::accelerator::Buffer<int> slots(tarch::MemoryLocation::Device);
  slots.setValues(std::vector<int>{3, 1});
  tarch::accelerator::Buffer<double> received(tarch::MemoryLocation::Device);
  received.setValues(std::vector<double>{100.0, 101.0, 200.0, 201.0});

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(2 * ChunkSize),
    peano4::communication::ScatterChunks<double>{slots.getData(), received.getData(), pool.getView(), ChunkSize}
  );

  for (int element = 0; element < ChunkSize; element++) {
    validateNumericalEqualsWithParams1(getChunkValue(pool, 3, element), 100.0 + element, element);
    validateNumericalEqualsWithParams1(getChunkValue(pool, 1, element), 200.0 + element, element);
    validateNumericalEqualsWithParams1(getChunkValue(pool, 0, element), getValueOf(0, element), element);
  }
}
