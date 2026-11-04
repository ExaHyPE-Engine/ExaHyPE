// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/ChunkedMemoryPool.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/testing/UnitTest.h"

namespace {
  TARGET_DECLARE
  /**
   * An odd chunk size makes the pool pad every chunk to the alignment.
   */
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

  bool isChunkFilled(const Pool& pool, const int slot, const double seed) {
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

UNIT_TEST("A pool without a reservation reports a capacity of zero") {
  GIVEN("A heap pool") {
    const Pool pool(ChunkSize, tarch::MemoryLocation::Heap);

    THEN("The pool is a heap pool without a capacity, a segment size, a segment or a byte") {
      VALIDATE_EQUALS(pool.getCapacity(), 0);
      VALIDATE_EQUALS(pool.getSegmentSize(), 0);
      VALIDATE_EQUALS(pool.getNumberOfSegments(), 0);
      VALIDATE_EQUALS(pool.getAllocatedBytes().getTotal(), 0);
      VALIDATE(pool.getLocation() == tarch::MemoryLocation::Heap);
    }
  }
}

UNIT_TEST("A reservation allocates whole segments and keeps every segment") {
  GIVEN("A heap pool") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);

    WHEN("The pool reserves four slots") {
      pool.reserve(4);

      THEN("The pool allocates one segment of the minimum segment size") {
        VALIDATE_EQUALS(pool.getSegmentSize(), MinimumSegmentSize);
        VALIDATE_EQUALS(pool.getNumberOfSegments(), 1);
        VALIDATE_EQUALS(pool.getCapacity(), MinimumSegmentSize);
      }

      AND_WHEN("The pool reserves one segment, one slot more, five segments and three slots in turn") {
        pool.reserve(MinimumSegmentSize);
        const int segmentsForOneSegment = pool.getNumberOfSegments();
        pool.reserve(MinimumSegmentSize + 1);
        const int segmentsForOneSlotMore = pool.getNumberOfSegments();
        const int capacityForOneSlotMore = pool.getCapacity();
        pool.reserve(5 * MinimumSegmentSize);
        const int segmentsForFiveSegments = pool.getNumberOfSegments();
        pool.reserve(3);
        const int segmentsForThreeSlots = pool.getNumberOfSegments();

        THEN("The pool grows by whole segments and keeps its five segments for three slots") {
          VALIDATE_EQUALS(segmentsForOneSegment, 1);
          VALIDATE_EQUALS(segmentsForOneSlotMore, 2);
          VALIDATE_EQUALS(capacityForOneSlotMore, 2 * MinimumSegmentSize);
          VALIDATE_EQUALS(segmentsForFiveSegments, 5);
          VALIDATE_EQUALS(segmentsForThreeSlots, 5);
        }

        AND_WHEN("Every chunk receives values that identify it") {
          for (int slot = 0; slot < pool.getCapacity(); slot++) {
            fillChunk(pool, slot, 3.0 * slot);
          }

          THEN("Every chunk stores its own values") {
            for (int slot = 0; slot < pool.getCapacity(); slot++) {
              VALIDATE(isChunkFilled(pool, slot, 3.0 * slot), slot);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The first reservation sets the segment size to a sixteenth of it, rounded up to a power of two") {
  GIVEN("Two heap pools") {
    Pool large(ChunkSize, tarch::MemoryLocation::Heap);
    Pool odd(ChunkSize, tarch::MemoryLocation::Heap);

    WHEN("The first pool reserves 16 times 256 slots and the second pool 16 times 200 slots plus one") {
      large.reserve(16 * 256);
      odd.reserve(16 * 200 + 1);

      THEN("Both pools allocate segments of 256 slots, 16 in the first and 13 in the second, enough for every slot") {
        VALIDATE_EQUALS(large.getSegmentSize(), 256);
        VALIDATE_EQUALS(large.getNumberOfSegments(), 16);
        VALIDATE_EQUALS(odd.getSegmentSize(), 256);
        VALIDATE_EQUALS(odd.getNumberOfSegments(), 13);
        VALIDATE(odd.getCapacity() >= 16 * 200 + 1);
      }
    }
  }
}

UNIT_TEST("The segment size of a first reservation never falls below the minimum segment size") {
  GIVEN("A heap pool") {
    Pool small(ChunkSize, tarch::MemoryLocation::Heap);

    WHEN("The pool reserves one slot") {
      small.reserve(1);

      THEN("The pool has the minimum segment size") { VALIDATE_EQUALS(small.getSegmentSize(), MinimumSegmentSize); }
    }
  }
}

UNIT_TEST("A growing pool keeps its chunks in place") {
  GIVEN("A heap pool of three slots whose chunks store values that identify them") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
    pool.reserve(3);
    for (int slot = 0; slot < 3; slot++) {
      fillChunk(pool, slot, 10.0 * slot);
    }
    const double* const firstChunk = pool.getChunk(0);

    WHEN("The pool reserves ten segments") {
      pool.reserve(10 * MinimumSegmentSize);

      THEN("The first chunk keeps its address, every chunk keeps its values and the pool allocates ten segments") {
        VALIDATE(pool.getChunk(0) == firstChunk);
        for (int slot = 0; slot < 3; slot++) {
          VALIDATE(isChunkFilled(pool, slot, 10.0 * slot), slot);
        }
        VALIDATE_EQUALS(pool.getNumberOfSegments(), 10);
      }
    }
  }
}

UNIT_TEST("Every chunk starts aligned at its own address and the first two chunks lie one padded chunk apart") {
  GIVEN("A heap pool that reserved three segments") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
    pool.reserve(3 * MinimumSegmentSize);

    WHEN("The test collects the address of every chunk") {
      std::set<const double*> addresses;
      for (int slot = 0; slot < pool.getCapacity(); slot++) {
        addresses.insert(pool.getChunk(slot));
      }

      THEN("Every address is aligned and unique and the first two chunks lie one padded chunk apart") {
        for (int slot = 0; slot < pool.getCapacity(); slot++) {
          VALIDATE_EQUALS(reinterpret_cast<std::uintptr_t>(pool.getChunk(slot)) % PEANO_WITH_ALIGNMENT, 0, slot);
        }
        VALIDATE_EQUALS(static_cast<int>(addresses.size()), pool.getCapacity());
        VALIDATE(pool.getChunk(1) - pool.getChunk(0) == pool.getPaddedChunkSize());
      }
    }
  }
}

UNIT_TEST("A pool stores chunks of floats and of integers") {
  GIVEN("A heap pool of two chunks of three floats and a heap pool of two chunks of seven integers") {
    tarch::accelerator::ChunkedMemoryPool<float> floatPool(3, tarch::MemoryLocation::Heap);
    floatPool.reserve(2);
    tarch::accelerator::ChunkedMemoryPool<int> integerPool(7, tarch::MemoryLocation::Heap);
    integerPool.reserve(2);

    WHEN("The last element of the second float chunk and of the first integer chunk receive a value") {
      floatPool.getChunk(1)[2]   = 2.5F;
      integerPool.getChunk(0)[6] = 42;

      THEN("Both elements store their values and the second integer chunk starts on the alignment") {
        VALIDATE_EQUALS(floatPool.getChunk(1)[2], 2.5F);
        VALIDATE_EQUALS(integerPool.getChunk(0)[6], 42);
        VALIDATE_EQUALS(reinterpret_cast<std::uintptr_t>(integerPool.getChunk(1)) % PEANO_WITH_ALIGNMENT, 0);
      }
    }
  }
}

UNIT_TEST("A view and a const view address every chunk of the pool") {
  GIVEN("A heap pool that reserved three segments") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
    pool.reserve(3 * MinimumSegmentSize);

    WHEN("The pool returns a view and, through a const reference, a const view") {
      const auto  view      = pool.getView();
      const auto& constPool = pool;
      const auto  constView = constPool.getView();

      THEN("Both views return the chunk of the pool for every slot") {
        for (int slot = 0; slot < pool.getCapacity(); slot++) {
          VALIDATE(view.getChunk(slot) == pool.getChunk(slot), slot);
          VALIDATE(constView.getChunk(slot) == constPool.getChunk(slot), slot);
        }
      }
    }
  }
}

UNIT_TEST("A kernel writes a device pool through its view and a reduction reads it back without a host access") {
  GIVEN("A device pool that reserved three segments") {
    constexpr int NumberOfSlots = 3 * MinimumSegmentSize;
    Pool          pool(ChunkSize, tarch::MemoryLocation::Device);
    pool.reserve(NumberOfSlots);

    THEN("The pool is a device pool") { VALIDATE(pool.getLocation() == tarch::MemoryLocation::Device); }

    WHEN("A kernel writes the slot number into every element and a reduction sums the elements") {
      writeSlotIntoEveryElement(pool, NumberOfSlots);
      const double sum = getSumOfElements(pool, NumberOfSlots);

      THEN("The sum is the chunk size times the sum of every slot number") {
        VALIDATE_EQUALS(sum, ChunkSize * static_cast<double>(NumberOfSlots) * (NumberOfSlots - 1) / 2.0);
      }
    }
  }
}

UNIT_TEST("A pool counts its segments and segment table in its memory location and its host arrays as host bytes") {
  GIVEN("A heap pool and a device pool") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
    Pool devicePool(ChunkSize, tarch::MemoryLocation::Device);

    THEN("The heap pool counts zero bytes") { VALIDATE_EQUALS(pool.getAllocatedBytes().getTotal(), 0); }

    WHEN("Both pools reserve four slots") {
      pool.reserve(4);
      devicePool.reserve(4);
      const auto segmentBytes = static_cast<std::size_t>(pool.getCapacity())
        * static_cast<std::size_t>(pool.getPaddedChunkSize()) * sizeof(double);
      const auto tableBytes     = sizeof(double*);
      const auto hostArrayBytes = sizeof(tarch::accelerator::Buffer<double>) + sizeof(double*);

      THEN("The segment of each pool and its table count in its location, the host arrays on the host") {
        VALIDATE_EQUALS(pool.getNumberOfSegments(), 1);
        VALIDATE_EQUALS(pool.getSegment(0).getSizeInBytes(), segmentBytes);
        VALIDATE_EQUALS(pool.getAllocatedBytes().host, segmentBytes + tableBytes + hostArrayBytes);
        VALIDATE_EQUALS(pool.getAllocatedBytes().device, 0);
#if defined(PEANO_WITH_GPU_BACKEND)
        VALIDATE_EQUALS(devicePool.getAllocatedBytes().device, segmentBytes + tableBytes);
        VALIDATE_EQUALS(devicePool.getAllocatedBytes().host, hostArrayBytes);
#else
        VALIDATE_EQUALS(devicePool.getAllocatedBytes().host, segmentBytes + tableBytes + hostArrayBytes);
        VALIDATE_EQUALS(devicePool.getAllocatedBytes().device, 0);
#endif
      }

      AND_WHEN("The heap pool releases its segments") {
        pool.release();

        THEN("The heap pool counts zero bytes") { VALIDATE_EQUALS(pool.getAllocatedBytes().getTotal(), 0); }
      }
    }
  }
}

UNIT_TEST("A move transfers the segments of a pool") {
  GIVEN("A heap pool of two slots whose second chunk stores values from eleven") {
    Pool source(ChunkSize, tarch::MemoryLocation::Heap);
    source.reserve(2);
    fillChunk(source, 1, 11.0);
    const double* const chunk = source.getChunk(1);

    WHEN("The test constructs a new pool from the moved pool") {
      Pool moved(std::move(source));

      THEN("The new pool allocates one segment with the second chunk in place, also for its view") {
        VALIDATE_EQUALS(moved.getNumberOfSegments(), 1);
        VALIDATE(moved.getChunk(1) == chunk);
        VALIDATE(isChunkFilled(moved, 1, 11.0));
        VALIDATE(moved.getView().getChunk(1) == chunk);
      }

      AND_WHEN("The test moves the new pool into a heap pool of one slot") {
        Pool target(ChunkSize, tarch::MemoryLocation::Heap);
        target.reserve(1);
        target = std::move(moved);

        THEN("The target allocates one segment with the second chunk in place") {
          VALIDATE_EQUALS(target.getNumberOfSegments(), 1);
          VALIDATE(target.getChunk(1) == chunk);
          VALIDATE(isChunkFilled(target, 1, 11.0));
        }
      }
    }
  }
}

UNIT_TEST("A release empties a pool and the pool reserves again afterwards") {
  GIVEN("A heap pool that reserved two segments") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Heap);
    pool.reserve(2 * MinimumSegmentSize);

    WHEN("The pool releases its segments") {
      pool.release();

      THEN("The capacity, the number of segments and the segment size of the pool are zero") {
        VALIDATE_EQUALS(pool.getCapacity(), 0);
        VALIDATE_EQUALS(pool.getNumberOfSegments(), 0);
        VALIDATE_EQUALS(pool.getSegmentSize(), 0);
      }

      AND_WHEN("The pool reserves one slot and its chunk receives values") {
        pool.reserve(1);
        fillChunk(pool, 0, 1.0);

        THEN("The pool allocates one segment and the chunk stores the values") {
          VALIDATE_EQUALS(pool.getNumberOfSegments(), 1);
          VALIDATE(isChunkFilled(pool, 0, 1.0));
        }
      }
    }
  }
}

UNIT_TEST("Host code reads and writes every chunk of a growing device pool through download() and upload()") {
  GIVEN("A device pool of one segment and one slot whose chunks store their slot number and a heap pool as copy") {
    Pool pool(ChunkSize, tarch::MemoryLocation::Device);
    pool.reserve(MinimumSegmentSize + 1);
    writeSlotIntoEveryElement(pool, pool.getCapacity());
    Pool copyOnHost(ChunkSize, tarch::MemoryLocation::Heap);

    WHEN("Host code downloads the pool through a const reference") {
      const Pool& chunks = std::as_const(pool).download(copyOnHost);

      THEN("The download returns a host-accessible pool itself and fills the copy otherwise, with every slot number") {
        VALIDATE(chunks.isHostAccessible());
        VALIDATE_EQUALS(&chunks == &pool, pool.isHostAccessible());
        VALIDATE_EQUALS(copyOnHost.getCapacity(), pool.isHostAccessible() ? 0 : pool.getCapacity());
        for (int slot = 0; slot < pool.getCapacity(); slot++) {
          const double* const chunk = chunks.getChunk(slot);
          for (int element = 0; element < ChunkSize; element++) {
            VALIDATE_EQUALS(chunk[element], static_cast<double>(slot), slot, element);
          }
        }
      }

      AND_WHEN("The pool grows by one slot and host code fills every chunk of a second download and uploads it") {
        pool.reserve(pool.getCapacity() + 1);
        Pool&     writableChunks        = pool.download(copyOnHost);
        const int capacityOfTheDownload = writableChunks.getCapacity();
        for (int slot = 0; slot < pool.getCapacity(); slot++) {
          fillChunk(writableChunks, slot, slot);
        }
        pool.upload(writableChunks);
        const auto   numberOfSlots = static_cast<double>(pool.getCapacity());
        const double expectedSum =
          ChunkSize * numberOfSlots * (numberOfSlots - 1.0) / 2.0 + numberOfSlots * ChunkSize * (ChunkSize - 1.0) / 2.0;
        const double sum = getSumOfElements(pool, pool.getCapacity());

        THEN("The second download covers every slot and a reduction over the pool sums the uploaded values") {
          VALIDATE_EQUALS(capacityOfTheDownload, pool.getCapacity());
          VALIDATE_NUMERICAL_EQUALS(sum, expectedSum);
        }
      }
    }
  }
}
