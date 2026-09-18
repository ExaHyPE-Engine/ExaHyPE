// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChunkedMemoryPoolTest.h"

#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/tests/TestMacros.h"

namespace {
  //! A cardinality that is not a multiple of the alignment block exercises the padding.
  constexpr int ChunkSize = 5;

  using Pool = tarch::accelerator::ChunkedMemoryPool<double, ChunkSize>;

  /** Writes a value that identifies the chunk into every element of it. */
  void fillChunk(Pool& pool, const Pool::SlotIndex slot, const double seed) {
    double* const chunk = pool.chunk(slot);
    for (int element = 0; element < ChunkSize; element++) {
      chunk[element] = seed + element;
    }
  }

  bool chunkHolds(const Pool& pool, const Pool::SlotIndex slot, const double seed) {
    const double* const chunk = pool.chunk(slot);
    for (int element = 0; element < ChunkSize; element++) {
      if (chunk[element] != seed + element) {
        return false;
      }
    }
    return true;
  }

  /** Returns whether the whole padded chunk reads as zero. */
  bool chunkIsZero(const Pool& pool, const Pool::SlotIndex slot) {
    const double* const chunk = pool.chunk(slot);
    return std::all_of(chunk, chunk + Pool::PaddedChunkSize, [](const double element) { return element == 0.0; });
  }
} // namespace

tarch::accelerator::tests::ChunkedMemoryPoolTest::ChunkedMemoryPoolTest() :
  TestCase("tarch::accelerator::tests::ChunkedMemoryPoolTest") {}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::run() {
  testMethod(testEmptyPool);
  testMethod(testResizeAndIndexedFill);
  testMethod(testResizeKeepsContent);
  testMethod(testFreshChunksReadAsZero);
  testMethod(testCapacityGrowsGeometrically);
  testMethod(testShrinkKeepsCapacity);
  testMethod(testChunkAlignment);
  testMethod(testScalarTypes);
  testMethod(testRawDataIsStrided);
  testMethod(testPinnedPlacement);
  testMethod(testAllocatedBytes);
  testMethod(testMoveTransfersBuffer);
  testMethod(testParallelDisjointFill);
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testEmptyPool() {
  const Pool pool;

  validate(pool.empty());
  validateEquals(pool.capacity(), 0);
  validateEquals(pool.rawSize(), 0);
  validateEquals(pool.allocatedBytes(), 0);
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testResizeAndIndexedFill() {
  Pool pool;
  pool.resize(4);

  validateEquals(pool.size(), 4);
  validate(not pool.empty());

  for (Pool::SlotIndex slot = 0; slot < 4; slot++) {
    fillChunk(pool, slot, 100.0 * slot);
  }
  for (Pool::SlotIndex slot = 0; slot < 4; slot++) {
    validate(chunkHolds(pool, slot, 100.0 * slot));
  }
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testResizeKeepsContent() {
  Pool pool;
  pool.resize(3);
  for (Pool::SlotIndex slot = 0; slot < 3; slot++) {
    fillChunk(pool, slot, 10.0 * slot);
  }

  // A growth beyond the capacity moves the buffer, so this is the case in
  // which the content has to be carried across.
  pool.resize(64);

  validateEquals(pool.size(), 64);
  for (Pool::SlotIndex slot = 0; slot < 3; slot++) {
    validate(chunkHolds(pool, slot, 10.0 * slot));
  }
}

/**
 * Every chunk that enters the live range reads as zero, a chunk that re-enters
 * it after a shrink included.
 */
void tarch::accelerator::tests::ChunkedMemoryPoolTest::testFreshChunksReadAsZero() {
  Pool pool;
  pool.resize(3);
  for (Pool::SlotIndex slot = 0; slot < 3; slot++) {
    validate(chunkIsZero(pool, slot));
  }

  fillChunk(pool, 2, 5.0);
  pool.resize(64);
  validate(chunkHolds(pool, 2, 5.0));
  validate(chunkIsZero(pool, 3));
  validate(chunkIsZero(pool, 63));

  fillChunk(pool, 63, 9.0);
  pool.resize(1);
  pool.resize(64);
  validate(chunkIsZero(pool, 2));
  validate(chunkIsZero(pool, 63));

  Pool pinned(tarch::MemoryLocation::Pinned);
  pinned.resize(2);
  validate(chunkIsZero(pinned, 0));
  validate(chunkIsZero(pinned, 1));
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testCapacityGrowsGeometrically() {
  Pool pool;
  pool.resize(100);
  const std::size_t capacityAfterFirstGrowth = pool.capacity();
  validate(capacityAfterFirstGrowth >= 100);

  // One chunk more than the capacity must not allocate one chunk more, or a
  // sequence of regrids would relocate the buffer every time.
  pool.resize(capacityAfterFirstGrowth + 1);
  validate(pool.capacity() > capacityAfterFirstGrowth + 1);
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testShrinkKeepsCapacity() {
  Pool pool;
  pool.resize(32);
  const std::size_t grownCapacity = pool.capacity();
  fillChunk(pool, 0, 7.0);

  pool.resize(2);

  validateEquals(pool.size(), 2);
  validateEquals(pool.capacity(), grownCapacity);
  validate(chunkHolds(pool, 0, 7.0));
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testChunkAlignment() {
  Pool pool;
  pool.resize(8);

  for (Pool::SlotIndex slot = 0; slot < 8; slot++) {
    const auto address = reinterpret_cast<std::uintptr_t>(pool.chunk(slot));
    validateEquals(address % PEANO_WITH_ALIGNMENT, 0);
  }

  tarch::accelerator::ChunkedMemoryPool<double, 64> alignedPool;
  alignedPool.resize(2);
  for (decltype(alignedPool)::SlotIndex slot = 0; slot < 2; slot++) {
    const auto address = reinterpret_cast<std::uintptr_t>(alignedPool.chunk(slot));
    validateEquals(address % PEANO_WITH_ALIGNMENT, 0);
  }
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testScalarTypes() {
  tarch::accelerator::ChunkedMemoryPool<float, ChunkSize> floatPool;
  floatPool.resize(1);
  floatPool.chunk(0)[0] = 1.5F;
  validateEquals(floatPool.chunk(0)[0], 1.5F);

  tarch::accelerator::ChunkedMemoryPool<char, ChunkSize> charPool;
  charPool.resize(1);
  charPool.chunk(0)[0] = 'a';
  validateEquals(charPool.chunk(0)[0], 'a');

  tarch::accelerator::ChunkedMemoryPool<int, ChunkSize> intPool;
  intPool.resize(1);
  intPool.chunk(0)[0] = 42;
  validateEquals(intPool.chunk(0)[0], 42);
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testRawDataIsStrided() {
  Pool pool;
  pool.resize(3);
  for (Pool::SlotIndex slot = 0; slot < 3; slot++) {
    fillChunk(pool, slot, 1000.0 * slot);
  }

  validateEquals(pool.rawSize(), 3 * static_cast<std::size_t>(Pool::PaddedChunkSize));

  const double* const buffer = pool.rawData();
  for (Pool::SlotIndex slot = 0; slot < 3; slot++) {
    validateEquals(
      buffer[static_cast<std::size_t>(slot) * static_cast<std::size_t>(Pool::PaddedChunkSize)],
      1000.0 * slot
    );
  }
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testPinnedPlacement() {
  Pool pool(tarch::MemoryLocation::Pinned);
  pool.resize(2);
  fillChunk(pool, 0, 3.0);

  validate(chunkHolds(pool, 0, 3.0));
  validateEquals(reinterpret_cast<std::uintptr_t>(pool.chunk(1)) % PEANO_WITH_ALIGNMENT, 0);
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testAllocatedBytes() {
  Pool pool;
  validateEquals(pool.allocatedBytes(), 0);

  pool.resize(4);
  validateEquals(
    pool.allocatedBytes(),
    pool.capacity() * static_cast<std::size_t>(Pool::PaddedChunkSize) * sizeof(double)
  );
  validateEquals(pool.rawSizeInBytes(), 4 * static_cast<std::size_t>(Pool::PaddedChunkSize) * sizeof(double));
}

void tarch::accelerator::tests::ChunkedMemoryPoolTest::testMoveTransfersBuffer() {
  Pool source;
  source.resize(2);
  fillChunk(source, 1, 11.0);

  const double* const buffer = source.rawData();

  Pool moved(std::move(source));

  validateEquals(moved.size(), 2);
  validate(moved.rawData() == buffer);
  validate(chunkHolds(moved, 1, 11.0));

  Pool target;
  target.resize(1);
  target = std::move(moved);

  validateEquals(target.size(), 2);
  validate(target.rawData() == buffer);
  validate(chunkHolds(target, 1, 11.0));

  target.clear();
  validate(target.empty());
  validateEquals(target.capacity(), 0);
}

/**
 * Concurrent iterations write disjoint chunks without a lock.
 */
void tarch::accelerator::tests::ChunkedMemoryPoolTest::testParallelDisjointFill() {
  constexpr int NumberOfChunks = 256;
  Pool          pool;
  pool.resize(NumberOfChunks);

  tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfChunks), [&pool](const int slot) {
    fillChunk(pool, static_cast<Pool::SlotIndex>(slot), 2.0 * slot);
  });

  for (int slot = 0; slot < NumberOfChunks; slot++) {
    validate(chunkHolds(pool, static_cast<Pool::SlotIndex>(slot), 2.0 * slot));
  }
}
