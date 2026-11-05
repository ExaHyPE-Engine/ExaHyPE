// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/communication/HaloExchangeKernels.h"

#include "peano4/CellConnectivity.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.h"

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
    std::vector<double> chunks          = tarch::accelerator::unittests::getChunksOf(pool);
    const auto          paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());
    for (int face = 0; face < NumberOfFaces; face++) {
      for (int element = 0; element < ChunkSize; element++) {
        chunks[static_cast<std::size_t>(face) * paddedChunkSize + static_cast<std::size_t>(element)] =
          getValueOf(face, element);
      }
    }
    tarch::accelerator::unittests::setChunksOf(pool, chunks);
    return pool;
  }
} // namespace

UNIT_TEST("The gather copies the half this rank owns of every listed face into the send buffer") {
  GIVEN("A filled pool of faces, the exchanged faces two on side one and zero on side zero and a send buffer") {
    const Pool                                        pool = getFilledPool();
    tarch::accelerator::Buffer<peano4::ExchangedFace> entries(tarch::MemoryLocation::Device);
    entries.setValues(std::vector<peano4::ExchangedFace>{{2, 1}, {0, 0}});
    tarch::accelerator::Buffer<double> send(2 * HalfSize, tarch::MemoryLocation::Device);

    WHEN("The gather kernel runs over the halves of the exchanged faces") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(2 * HalfSize),
        peano4::communication::GatherHaloHalves<double>{pool.getView(), entries.getData(), send.getData(), HalfSize}
      );
      const std::vector<double> sent = send.getValues();

      THEN("The gather copies the second half of face two and the first half of face zero in this order") {
        for (int element = 0; element < HalfSize; element++) {
          VALIDATE_NUMERICAL_EQUALS(
            sent[static_cast<std::size_t>(element)],
            getValueOf(2, HalfSize + element),
            element
          );
          VALIDATE_NUMERICAL_EQUALS(
            sent[static_cast<std::size_t>(HalfSize + element)],
            getValueOf(0, element),
            element
          );
        }
      }
    }
  }
}

UNIT_TEST("The scatter writes every received half into the side of its face that the neighbour owns") {
  GIVEN("A filled pool of faces, the exchanged faces one on side zero and three on side one and two received halves") {
    Pool                                              pool = getFilledPool();
    tarch::accelerator::Buffer<peano4::ExchangedFace> entries(tarch::MemoryLocation::Device);
    entries.setValues(std::vector<peano4::ExchangedFace>{{1, 0}, {3, 1}});
    tarch::accelerator::Buffer<double> received(tarch::MemoryLocation::Device);
    received.setValues(std::vector<double>{100.0, 101.0, 102.0, 200.0, 201.0, 202.0});

    WHEN("The scatter kernel runs over the received halves and the test copies the pool to the host") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(2 * HalfSize),
        peano4::communication::ScatterHaloHalves<double>{
          entries.getData(),
          received.getData(),
          pool.getView(),
          HalfSize
        }
      );
      const std::vector<double> chunks          = tarch::accelerator::unittests::getChunksOf(pool);
      const auto                paddedChunkSize = static_cast<std::size_t>(pool.getPaddedChunkSize());

      THEN("The received halves fill the second half of face one and the first half of face three") {
        for (int element = 0; element < HalfSize; element++) {
          const auto index = static_cast<std::size_t>(element);
          VALIDATE_NUMERICAL_EQUALS(chunks[paddedChunkSize + HalfSize + index], 100.0 + element, element);
          VALIDATE_NUMERICAL_EQUALS(chunks[3 * paddedChunkSize + index], 200.0 + element, element);
        }
      }

      THEN("The first half of face one and the second half of face three keep their values") {
        for (int element = 0; element < HalfSize; element++) {
          const auto index = static_cast<std::size_t>(element);
          VALIDATE_NUMERICAL_EQUALS(chunks[paddedChunkSize + index], getValueOf(1, element), element);
          VALIDATE_NUMERICAL_EQUALS(
            chunks[3 * paddedChunkSize + HalfSize + index],
            getValueOf(3, HalfSize + element),
            element
          );
        }
      }
    }
  }
}
