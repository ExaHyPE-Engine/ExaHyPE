// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsageTest.h"

#include "peano4/data/MemoryUsage.h"
#include "tarch/mpi/Rank.h"

namespace {
  struct Record final {
    using DoFType                    = double;
    static constexpr int Cardinality = 4;
  };

  struct Pool final {
    [[nodiscard]] std::size_t allocatedBytes() const { return bytes; }
    void                      clear() { bytes = 0; }
    std::size_t               bytes{0};
  };

  struct PooledRecord final {
    static Pool& getMemoryPool() {
      static Pool pool;
      return pool;
    }
  };

  std::uint64_t summedOverRanks(const std::uint64_t localValue) {
    return localValue * static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
  }
} // namespace

peano4::data::tests::MemoryUsageTest::MemoryUsageTest() :
  TestCase("peano4::data::tests::MemoryUsageTest") {}

void peano4::data::tests::MemoryUsageTest::run() {
  testMethod(testPayloadBytesOfRecordsAndPools);
  testMethod(testReleasePoolStorageClearsOnlyAPool);
  testMethod(testTheCountersLagOneUpdate);
}

/**
 * Checks that a record type counts its slots and a pooled type counts the bytes of its pool.
 */
void peano4::data::tests::MemoryUsageTest::testPayloadBytesOfRecordsAndPools() {
  validateEquals(peano4::data::payloadBytesOf<Record>(3), static_cast<std::uint64_t>(3 * 4 * sizeof(double)));
  validateEquals(peano4::data::payloadBytesOf<Record>(0), static_cast<std::uint64_t>(0));

  PooledRecord::getMemoryPool().bytes = 96;
  validateEquals(peano4::data::payloadBytesOf<PooledRecord>(1000), static_cast<std::uint64_t>(96));
  PooledRecord::getMemoryPool().bytes = 0;
}

/**
 * Checks that the release clears the pool of a pooled type and leaves a record type alone.
 */
void peano4::data::tests::MemoryUsageTest::testReleasePoolStorageClearsOnlyAPool() {
  PooledRecord::getMemoryPool().bytes = 64;
  peano4::data::releasePoolStorage<Record>();
  validateEquals(PooledRecord::getMemoryPool().bytes, static_cast<std::size_t>(64));
  peano4::data::releasePoolStorage<PooledRecord>();
  validateEquals(PooledRecord::getMemoryPool().bytes, static_cast<std::size_t>(0));
}

/**
 * Checks that the first update reports the bytes of its own rank and the second update the sums
 * of the first one over all ranks.
 */
void peano4::data::tests::MemoryUsageTest::testTheCountersLagOneUpdate() {
  peano4::data::MemoryUsage::update(1, 2, 3);
  validateEquals(peano4::data::MemoryUsage::getGlobalCellDataMemoryUsageInBytes(), static_cast<std::size_t>(1));
  validateEquals(peano4::data::MemoryUsage::getGlobalFaceDataMemoryUsageInBytes(), static_cast<std::size_t>(2));
  validateEquals(peano4::data::MemoryUsage::getGlobalVertexDataMemoryUsageInBytes(), static_cast<std::size_t>(3));

  peano4::data::MemoryUsage::update(40, 50, 60);
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    validateEquals(
      peano4::data::MemoryUsage::getGlobalCellDataMemoryUsageInBytes(),
      static_cast<std::size_t>(summedOverRanks(1))
    );
    validateEquals(
      peano4::data::MemoryUsage::getGlobalFaceDataMemoryUsageInBytes(),
      static_cast<std::size_t>(summedOverRanks(2))
    );
    validateEquals(
      peano4::data::MemoryUsage::getGlobalVertexDataMemoryUsageInBytes(),
      static_cast<std::size_t>(summedOverRanks(3))
    );
  } else {
    validateEquals(peano4::data::MemoryUsage::getGlobalCellDataMemoryUsageInBytes(), static_cast<std::size_t>(40));
    validateEquals(peano4::data::MemoryUsage::getGlobalFaceDataMemoryUsageInBytes(), static_cast<std::size_t>(50));
    validateEquals(peano4::data::MemoryUsage::getGlobalVertexDataMemoryUsageInBytes(), static_cast<std::size_t>(60));
  }
}
