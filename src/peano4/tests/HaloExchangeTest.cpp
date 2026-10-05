// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HaloExchangeTest.h"

#include "peano4/HaloExchange.h"
#include "peano4/Partition.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

namespace {
  TARGET_DECLARE
  constexpr int    Depth           = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    ElementsPerHalf = 5;
  constexpr int    ElementsPerFace = 2 * ElementsPerHalf;
  constexpr int    LatticeRange    = 4096;
  constexpr double Untouched       = -1.0;
  TARGET_DECLARE_END

  using Pool     = tarch::accelerator::ChunkedMemoryPool<double>;
  using Exchange = peano4::HaloExchange<double>;

  /**
   * Returns the value the owner of side writes into element of a face. Both ranks derive it from the
   * level, normal and lattice position of the face.
   */
  GPU_CALLABLE_METHOD double getExpectedValue(
    const peano4::Partition& partition,
    const int                face,
    const int                side,
    const int                element,
    const int                round
  ) {
    std::int64_t key = partition.getFaceLevel(face) * DIMENSIONS + partition.getFaceNormal(face);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * LatticeRange + partition.getFaceLattice(face)(axis);
    }
    return static_cast<double>((key * 2 + side) * ElementsPerFace + element + round);
  }

  /**
   * Returns for every face the side this rank owns, or -1 for a face that no neighbour rank holds.
   */
  std::vector<int> getLocalSidesOfFaces(const peano4::Partition& partition) {
    std::vector<int> sides(static_cast<std::size_t>(partition.getNumberOfFaces()), -1);
    for (const auto& neighbour : partition.getNeighbours()) {
      for (const auto& entry : partition.getExchangedFaces(neighbour)) {
        sides[static_cast<std::size_t>(entry.face)] = entry.side;
      }
    }
    return sides;
  }

  int getNumberOfExchangedFaces(const peano4::Partition& partition) {
    int faces = 0;
    for (const auto& neighbour : partition.getNeighbours()) {
      faces += partition.getExchangedFaces(neighbour).size;
    }
    return faces;
  }

  TARGET_DECLARE
  /**
   * Writes the marker into one face and the expected value into the half of it that this rank owns.
   */
  struct WriteLocalHalf final {
    peano4::Partition partition{};
    Pool::View        faces{};
    const int*        sideOfFace{nullptr};
    int               round{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      double* const chunk = faces.getChunk(face);
      for (int element = 0; element < ElementsPerFace; element++) {
        chunk[element] = Untouched;
      }
      const int side = sideOfFace[face];
      if (side >= 0) {
        for (int element = 0; element < ElementsPerHalf; element++) {
          chunk[side * ElementsPerHalf + element] = getExpectedValue(partition, face, side, element, round);
        }
      }
    }
  };

  /**
   * Writes the number of elements of one face that differ from the expected value of their side
   * on an exchanged face and from the marker on every other face.
   */
  struct CountMismatchesOfFace final {
    peano4::Partition partition{};
    Pool::ConstView   faces{};
    const int*        sideOfFace{nullptr};
    int               round{0};
    int*              mismatchesOfFace{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const double* const chunk      = faces.getChunk(face);
      const int           side       = sideOfFace[face];
      int                 mismatches = 0;
      for (int element = 0; element < ElementsPerFace; element++) {
        double expected = Untouched;
        if (side >= 0) {
          const int elementSide = element / ElementsPerHalf;
          expected = getExpectedValue(partition, face, elementSide, element - elementSide * ElementsPerHalf, round);
        }
        if (chunk[element] != expected) {
          mismatches++;
        }
      }
      mismatchesOfFace[face] = mismatches;
    }
  };
  TARGET_DECLARE_END

  /**
   * Writes the marker into every face and the expected value into the half of every exchanged face
   * that this rank owns.
   */
  void writeLocalHalves(
    Pool&                                  pool,
    const peano4::Partition&               partition,
    const tarch::accelerator::Buffer<int>& sides,
    const int                              round
  ) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(partition.getNumberOfFaces()),
      WriteLocalHalf{partition.onDevice(), pool.getView(), sides.getData(), round}
    );
  }

  /**
   * Returns the number of face elements that differ from the expected value on an exchanged face or
   * from the marker elsewhere.
   */
  int getNumberOfMismatches(
    const Pool&                            pool,
    const peano4::Partition&               partition,
    const tarch::accelerator::Buffer<int>& sides,
    const int                              round
  ) {
    const int                       numberOfFaces = partition.getNumberOfFaces();
    tarch::accelerator::Buffer<int> mismatchesOfFace(static_cast<std::size_t>(numberOfFaces), pool.getLocation());
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(numberOfFaces),
      CountMismatchesOfFace{partition.onDevice(), pool.getView(), sides.getData(), round, mismatchesOfFace.getData()}
    );
    const std::vector<int> mismatches = mismatchesOfFace.getValues();
    return std::accumulate(mismatches.begin(), mismatches.end(), 0);
  }
} // namespace

peano4::tests::HaloExchangeTest::HaloExchangeTest() :
  tarch::tests::TestCase("peano4::tests::HaloExchangeTest") {}

void
peano4::tests::HaloExchangeTest::run() {
  testMethod(testExchangeFillsTheRemoteHalves);
  testMethod(testExchangesHoldDistinctTags);
  testMethod(testReleaseFreesTheBuffersAndKeepsTheTag);
}

/**
 * After every exchange on a regular, a refined and a periodic grid, the remote half of every
 * exchanged face stores the value of the partner rank and every other face keeps its marker.
 */
void
peano4::tests::HaloExchangeTest::testExchangeFillsTheRemoteHalves() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int rank          = tarch::mpi::Rank::getInstance().getRank();

  Pool     pool(ElementsPerFace, tarch::MemoryLocation::Device);
  Exchange exchange(pool);

  for (const int grid : {0, 1, 2}) {
    const bool              refined  = grid == 1;
    const bool              periodic = grid == 2;
    std::bitset<DIMENSIONS> periodicBoundaries;
    if (periodic) {
      periodicBoundaries.set();
    }
    peano4::tests::buildRegularGrid(Depth, periodicBoundaries);
    if (refined) {
      peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
    }
    const auto& partition = peano4::Spacetree::getInstance().getPartition();

    pool.reserve(partition.getNumberOfFaces());
    exchange.rebuild(partition);

    const int  numberOfExchangedFaces = getNumberOfExchangedFaces(partition);
    const bool hasNoEntries           = numberOfExchangedFaces == 0;
    const bool runsAlone              = numberOfRanks == 1;
    validateEqualsWithParams1(exchange.getNumberOfEntries(), numberOfExchangedFaces, grid);
    validateEqualsWithParams1(hasNoEntries, runsAlone, grid);
    validateWithParams1(
      exchange.getAllocatedBytes().getTotal()
        >= static_cast<std::size_t>(numberOfExchangedFaces) * sizeof(peano4::ExchangedFace),
      grid
    );

    tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
    sides.setValues(getLocalSidesOfFaces(partition));

    for (const int round : {0, 1}) {
      writeLocalHalves(pool, partition, sides, round);

      exchange.post();
      exchange.wait();

      validateEqualsWithParams3(getNumberOfMismatches(pool, partition, sides, round), 0, grid, round, rank);
    }
  }
}

/**
 * Two exchanges of one pool get distinct tags in their first rebuild, and every rank reserves the
 * same tags.
 */
void
peano4::tests::HaloExchangeTest::testExchangesHoldDistinctTags() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& partition = peano4::Spacetree::getInstance().getPartition();
  Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
  Exchange    first(pool);
  Exchange    second(pool);
  validateEquals(first.getAllocatedBytes().getTotal(), std::size_t{0});
  pool.reserve(partition.getNumberOfFaces());
  first.rebuild(partition);
  second.rebuild(partition);
  validate(first.getTag() != second.getTag());
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
 * The release frees the buffers and keeps the tag, and the next rebuild restores a working exchange.
 */
void
peano4::tests::HaloExchangeTest::testReleaseFreesTheBuffersAndKeepsTheTag() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& partition = peano4::Spacetree::getInstance().getPartition();
  Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
  pool.reserve(partition.getNumberOfFaces());
  Exchange exchange(pool);
  exchange.rebuild(partition);
  const int tag             = exchange.getTag();
  const int numberOfEntries = exchange.getNumberOfEntries();

  exchange.release();
  validateEquals(exchange.getAllocatedBytes().getTotal(), std::size_t{0});
  validateEquals(exchange.getNumberOfEntries(), 0);
  validateEquals(exchange.getTag(), tag);

  exchange.rebuild(partition);
  validateEquals(exchange.getNumberOfEntries(), numberOfEntries);
  tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
  sides.setValues(getLocalSidesOfFaces(partition));
  writeLocalHalves(pool, partition, sides, 0);
  exchange.post();
  exchange.wait();
  validateEquals(getNumberOfMismatches(pool, partition, sides, 0), 0);
}
