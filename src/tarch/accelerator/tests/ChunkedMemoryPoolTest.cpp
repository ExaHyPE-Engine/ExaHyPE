// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChunkedMemoryPoolTest.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/tests/TestMacros.h"

namespace {
  TARGET_DECLARE
  //! A chunk size that is no multiple of the alignment makes the pool pad every chunk.
  constexpr int ChunkSize = 5;
  TARGET_DECLARE_END

  using Pool = tarch::accelerator::ChunkedMemoryPool<double>;

  constexpr int MinimumSegmentSize = 1 << Pool::MinimumSegmentSizeExponent;

  void fillChunk(Pool& pool, const int slot, const double seed) {
    double* const chunk = pool.getChunk(slot);
    for (int element = 0; element < ChunkSize; element++) {
      chunk[element] = seed + element;
    }
  }

  bool chunkHolds(const Pool& pool, const int slot, const double seed) {
    const double* const chunk = pool.getChunk(slot);
    for (int element = 0; element < ChunkSize; element++) {
      if (chunk[element] != seed + element) {
        return false;
      }
    }
    return true;
  }

  TARGET_DECLARE
  struct WriteSlotIntoEveryElement final {
    Pool::View chunks{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int slot) const {
      double* const chunk = chunks.getChunk(slot);
      for (int element = 0; element < ChunkSize; element++) {
        chunk[element] = static_cast<double>(slot);
      }
    }
  };

  struct AddElementsOfChunk final {
    Pool::ConstView chunks{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int slot, double& accumulator) const {
      const double* const chunk = chunks.getChunk(slot);
      for (int element = 0; element < ChunkSize; element++) {
        accumulator += chunk[element];
      }
    }
  };
  TARGET_DECLARE_END

  void writeSlotIntoEveryElement(Pool& pool, const int numberOfSlots) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(numberOfSlots),
      WriteSlotIntoEveryElement{pool.getView()}
    );
  }

  double getSumOfElements(const Pool& pool, const int numberOfSlots) {
    double sum = 0.0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(numberOfSlots),
      AddElementsOfChunk{pool.getView()},
      tarch::accelerator::Sum<double>{sum}
    );
    return sum;
  }
} // namespace

tarch::accelerator::tests::ChunkedMemoryPoolTest::ChunkedMemoryPoolTest() :
  TestCase("tarch::accelerator::tests::ChunkedMemoryPoolTest") {}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::run() {
  testMethod(testEmptyPool);
  testMethod(testReserveAllocatesWholeSegments);
  testMethod(testSegmentSizeFollowsTheFirstReservation);
  testMethod(testGrowthKeepsChunksInPlace);
  testMethod(testChunkAlignment);
  testMethod(testScalarTypes);
  testMethod(testViewAddressesEveryChunk);
  testMethod(testDeviceFillThroughView);
  testMethod(testAllocatedBytes);
  testMethod(testMoveTransfersSegments);
  testMethod(testRelease);
  testMethod(testHostCopiesRoundTrip);
  testMethod(testHostCodeReadsAndWritesEveryChunkThroughDownloadAndUpload);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testEmptyPool() {
  const Pool pool(ChunkSize, tarch::MemoryLocation::Heap);

  validateEquals(pool.getCapacity(), 0);
  validateEquals(pool.getSegmentSize(), 0);
  validateEquals(pool.getNumberOfSegments(), 0);
  validateEquals(pool.getAllocatedBytes().getTotal(), 0);
  validate(pool.getLocation() == tarch::MemoryLocation::Heap);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testReserveAllocatesWholeSegments() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);

  pool.reserve(4);
  validateEquals(pool.getSegmentSize(), MinimumSegmentSize);
  validateEquals(pool.getNumberOfSegments(), 1);
  validateEquals(pool.getCapacity(), MinimumSegmentSize);

  pool.reserve(MinimumSegmentSize);
  validateEquals(pool.getNumberOfSegments(), 1);

  pool.reserve(MinimumSegmentSize + 1);
  validateEquals(pool.getNumberOfSegments(), 2);
  validateEquals(pool.getCapacity(), 2 * MinimumSegmentSize);

  pool.reserve(5 * MinimumSegmentSize);
  validateEquals(pool.getNumberOfSegments(), 5);

  pool.reserve(3);
  validateEquals(pool.getNumberOfSegments(), 5);

  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    fillChunk(pool, slot, 3.0 * slot);
  }
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    validateWithParams1(chunkHolds(pool, slot, 3.0 * slot), slot);
  }
}

/**
 * The first reservation sizes a segment to the smallest power of two at or above both a sixteenth
 * of the reserved slots and the minimum.
 */
void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testSegmentSizeFollowsTheFirstReservation() {
  Pool large(ChunkSize, tarch::MemoryLocation::Heap);
  large.reserve(16 * 256);
  validateEquals(large.getSegmentSize(), 256);
  validateEquals(large.getNumberOfSegments(), 16);

  Pool odd(ChunkSize, tarch::MemoryLocation::Heap);
  odd.reserve(16 * 200 + 1);
  validateEquals(odd.getSegmentSize(), 256);
  validateEquals(odd.getNumberOfSegments(), 13);
  validate(odd.getCapacity() >= 16 * 200 + 1);

  Pool small(ChunkSize, tarch::MemoryLocation::Heap);
  small.reserve(1);
  validateEquals(small.getSegmentSize(), MinimumSegmentSize);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testGrowthKeepsChunksInPlace() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  pool.reserve(3);
  for (int slot = 0; slot < 3; slot++) {
    fillChunk(pool, slot, 10.0 * slot);
  }
  const double* const firstChunk = pool.getChunk(0);

  pool.reserve(10 * MinimumSegmentSize);

  validate(pool.getChunk(0) == firstChunk);
  for (int slot = 0; slot < 3; slot++) {
    validateWithParams1(chunkHolds(pool, slot, 10.0 * slot), slot);
  }
  validateEquals(pool.getNumberOfSegments(), 10);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testChunkAlignment() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  pool.reserve(3 * MinimumSegmentSize);

  std::set<const double*> addresses;
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    validateEqualsWithParams1(reinterpret_cast<std::uintptr_t>(pool.getChunk(slot)) % PEANO_WITH_ALIGNMENT, 0, slot);
    addresses.insert(pool.getChunk(slot));
  }
  validateEquals(static_cast<int>(addresses.size()), pool.getCapacity());
  validate(pool.getChunk(1) - pool.getChunk(0) == pool.getPaddedChunkSize());
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testScalarTypes() {
  tarch::accelerator::ChunkedMemoryPool<float> floatPool(3, tarch::MemoryLocation::Heap);
  floatPool.reserve(2);
  floatPool.getChunk(1)[2] = 2.5F;
  validateEquals(floatPool.getChunk(1)[2], 2.5F);

  tarch::accelerator::ChunkedMemoryPool<int> integerPool(7, tarch::MemoryLocation::Heap);
  integerPool.reserve(2);
  integerPool.getChunk(0)[6] = 42;
  validateEquals(integerPool.getChunk(0)[6], 42);
  validateEquals(reinterpret_cast<std::uintptr_t>(integerPool.getChunk(1)) % PEANO_WITH_ALIGNMENT, 0);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testViewAddressesEveryChunk() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  pool.reserve(3 * MinimumSegmentSize);

  const auto  view      = pool.getView();
  const auto& constPool = pool;
  const auto  constView = constPool.getView();
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    validateWithParams1(view.getChunk(slot) == pool.getChunk(slot), slot);
    validateWithParams1(constView.getChunk(slot) == constPool.getChunk(slot), slot);
  }
}

/**
 * A reduction reads back the chunks that a kernel writes through the view of a device pool.
 */
void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testDeviceFillThroughView() {
  constexpr int NumberOfSlots = 3 * MinimumSegmentSize;
  Pool          pool(ChunkSize, tarch::MemoryLocation::Device);
  pool.reserve(NumberOfSlots);
  validate(pool.getLocation() == tarch::MemoryLocation::Device);

  writeSlotIntoEveryElement(pool, NumberOfSlots);
  validateEquals(
    getSumOfElements(pool, NumberOfSlots),
    ChunkSize * static_cast<double>(NumberOfSlots) * (NumberOfSlots - 1) / 2.0
  );
}

/**
 * A pool counts its segments and its segment table in its memory location and its host arrays as host bytes until
 * release() frees them.
 */
void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testAllocatedBytes() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  validateEquals(pool.getAllocatedBytes().getTotal(), 0);

  pool.reserve(4);
  validateEquals(pool.getNumberOfSegments(), 1);
  const auto segmentBytes =
    static_cast<std::size_t>(pool.getCapacity()) * static_cast<std::size_t>(pool.getPaddedChunkSize()) * sizeof(double);
  const auto tableBytes     = sizeof(double*);
  const auto hostArrayBytes = sizeof(tarch::accelerator::Buffer<double>) + sizeof(double*);
  validateEquals(pool.getSegment(0).getSizeInBytes(), segmentBytes);
  validateEquals(pool.getAllocatedBytes().host, segmentBytes + tableBytes + hostArrayBytes);
  validateEquals(pool.getAllocatedBytes().device, 0);

  Pool devicePool(ChunkSize, tarch::MemoryLocation::Device);
  devicePool.reserve(4);
#if defined(PEANO_WITH_GPU_BACKEND)
  validateEquals(devicePool.getAllocatedBytes().device, segmentBytes + tableBytes);
  validateEquals(devicePool.getAllocatedBytes().host, hostArrayBytes);
#else
  validateEquals(devicePool.getAllocatedBytes().host, segmentBytes + tableBytes + hostArrayBytes);
  validateEquals(devicePool.getAllocatedBytes().device, 0);
#endif

  pool.release();
  validateEquals(pool.getAllocatedBytes().getTotal(), 0);
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testMoveTransfersSegments() {
  Pool source(ChunkSize, tarch::MemoryLocation::Heap);
  source.reserve(2);
  fillChunk(source, 1, 11.0);
  const double* const chunk = source.getChunk(1);

  Pool moved(std::move(source));
  validateEquals(moved.getNumberOfSegments(), 1);
  validate(moved.getChunk(1) == chunk);
  validate(chunkHolds(moved, 1, 11.0));
  validate(moved.getView().getChunk(1) == chunk);

  Pool target(ChunkSize, tarch::MemoryLocation::Heap);
  target.reserve(1);
  target = std::move(moved);
  validateEquals(target.getNumberOfSegments(), 1);
  validate(target.getChunk(1) == chunk);
  validate(chunkHolds(target, 1, 11.0));
}

void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testRelease() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  pool.reserve(2 * MinimumSegmentSize);
  pool.release();

  validateEquals(pool.getCapacity(), 0);
  validateEquals(pool.getNumberOfSegments(), 0);
  validateEquals(pool.getSegmentSize(), 0);

  pool.reserve(1);
  validateEquals(pool.getNumberOfSegments(), 1);
  fillChunk(pool, 0, 1.0);
  validate(chunkHolds(pool, 0, 1.0));
}

/**
 * copyToHost() writes the padded chunks of every slot into one vector and copyFromHost() reads them back.
 */
void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testHostCopiesRoundTrip() {
  Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
  pool.reserve(2 * MinimumSegmentSize + 1);
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    fillChunk(pool, slot, slot);
  }

  std::vector<double> host;
  pool.copyToHost(host);
  const auto valuesPerSlot = static_cast<std::size_t>(pool.getPaddedChunkSize());
  validateEquals(host.size(), static_cast<std::size_t>(pool.getCapacity()) * valuesPerSlot);
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    for (int element = 0; element < ChunkSize; element++) {
      validateEqualsWithParams2(
        host[static_cast<std::size_t>(slot) * valuesPerSlot + element],
        slot + element,
        slot,
        element
      );
      host[static_cast<std::size_t>(slot) * valuesPerSlot + element] += 1.0;
    }
  }

  pool.copyFromHost(host);
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    validateWithParams1(chunkHolds(pool, slot, slot + 1.0), slot);
  }
}

/**
 * Host code reads and writes every chunk of a growing device pool through download() and upload().
 */
void
tarch::accelerator::tests::ChunkedMemoryPoolTest::testHostCodeReadsAndWritesEveryChunkThroughDownloadAndUpload() {
  Pool pool(ChunkSize);
  pool.reserve(MinimumSegmentSize + 1);
  writeSlotIntoEveryElement(pool, pool.getCapacity());
  Pool copyOnHost(ChunkSize, tarch::MemoryLocation::Heap);

  const Pool& chunks = std::as_const(pool).download(copyOnHost);
  validate(chunks.isHostAccessible());
  validateEquals(&chunks == &pool, pool.isHostAccessible());
  validateEquals(copyOnHost.getCapacity(), pool.isHostAccessible() ? 0 : pool.getCapacity());
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    const double* const chunk = chunks.getChunk(slot);
    for (int element = 0; element < ChunkSize; element++) {
      validateEqualsWithParams2(chunk[element], static_cast<double>(slot), slot, element);
    }
  }

  pool.reserve(pool.getCapacity() + 1);
  Pool& writableChunks = pool.download(copyOnHost);
  validateEquals(writableChunks.getCapacity(), pool.getCapacity());
  for (int slot = 0; slot < pool.getCapacity(); slot++) {
    fillChunk(writableChunks, slot, slot);
  }
  pool.upload(writableChunks);

  const auto   numberOfSlots = static_cast<double>(pool.getCapacity());
  const double expectedSum =
    ChunkSize * numberOfSlots * (numberOfSlots - 1.0) / 2.0 + numberOfSlots * ChunkSize * (ChunkSize - 1.0) / 2.0;
  validateNumericalEquals(getSumOfElements(pool, pool.getCapacity()), expectedSum);
}
