// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HaloExchangeKernelsTest.h"

#include "peano4/CellConnectivity.h"
#include "peano4/communication/HaloExchangeKernels.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int NumberOfFaces = 4;
  constexpr int HalfSize      = 3;
  constexpr int ChunkSize     = 2 * HalfSize;

  using Pool = tarch::accelerator::ChunkedMemoryPool<double>;

  /**
   * Returns the value the test writes into element of the face.
   */
  double getValueOf(const int face, const int element) { return 10.0 * face + element; }

  Pool getFilledPool() {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    pool.reserve(NumberOfFaces);
    std::vector<double> chunks;
    pool.copyToHost(chunks);
    const auto paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());
    for (int face = 0; face < NumberOfFaces; face++) {
      for (int element = 0; element < ChunkSize; element++) {
        chunks[static_cast<std::size_t>(face) * paddedChunkSize + static_cast<std::size_t>(element)] =
          getValueOf(face, element);
      }
    }
    pool.copyFromHost(chunks);
    return pool;
  }
} // namespace

peano4::communication::tests::HaloExchangeKernelsTest::HaloExchangeKernelsTest() :
  tarch::tests::TestCase("peano4::communication::tests::HaloExchangeKernelsTest") {}

void
peano4::communication::tests::HaloExchangeKernelsTest::run() {
  testMethod(testGatherCopiesTheOwnHalfOfEveryListedFace);
  testMethod(testScatterFillsTheHalfTheNeighbourOwns);
}

void
peano4::communication::tests::HaloExchangeKernelsTest::testGatherCopiesTheOwnHalfOfEveryListedFace() {
  const Pool                                        pool = getFilledPool();
  tarch::accelerator::Buffer<peano4::ExchangedFace> entries(tarch::MemoryLocation::Device);
  entries.setValues(std::vector<peano4::ExchangedFace>{{2, 1}, {0, 0}});
  tarch::accelerator::Buffer<double> send(2 * HalfSize, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(2 * HalfSize),
    peano4::communication::GatherHaloHalves<double>{pool.getView(), entries.getData(), send.getData(), HalfSize}
  );

  const std::vector<double> sent = send.getValues();
  for (int element = 0; element < HalfSize; element++) {
    validateNumericalEqualsWithParams1(
      sent[static_cast<std::size_t>(element)],
      getValueOf(2, HalfSize + element),
      element
    );
    validateNumericalEqualsWithParams1(
      sent[static_cast<std::size_t>(HalfSize + element)],
      getValueOf(0, element),
      element
    );
  }
}

void
peano4::communication::tests::HaloExchangeKernelsTest::testScatterFillsTheHalfTheNeighbourOwns() {
  Pool                                              pool = getFilledPool();
  tarch::accelerator::Buffer<peano4::ExchangedFace> entries(tarch::MemoryLocation::Device);
  entries.setValues(std::vector<peano4::ExchangedFace>{{1, 0}, {3, 1}});
  tarch::accelerator::Buffer<double> received(tarch::MemoryLocation::Device);
  received.setValues(std::vector<double>{100.0, 101.0, 102.0, 200.0, 201.0, 202.0});

  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(2 * HalfSize),
    peano4::communication::ScatterHaloHalves<double>{entries.getData(), received.getData(), pool.getView(), HalfSize}
  );

  std::vector<double> chunks;
  pool.copyToHost(chunks);
  const auto paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());
  for (int element = 0; element < HalfSize; element++) {
    const auto index = static_cast<std::size_t>(element);
    validateNumericalEqualsWithParams1(chunks[paddedChunkSize + HalfSize + index], 100.0 + element, element);
    validateNumericalEqualsWithParams1(chunks[paddedChunkSize + index], getValueOf(1, element), element);
    validateNumericalEqualsWithParams1(chunks[3 * paddedChunkSize + index], 200.0 + element, element);
    validateNumericalEqualsWithParams1(
      chunks[3 * paddedChunkSize + HalfSize + index],
      getValueOf(3, HalfSize + element),
      element
    );
  }
}
