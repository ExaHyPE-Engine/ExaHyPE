// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HaloExchangeTest.h"

#include "peano4/CellConnectivity.h"
#include "peano4/HaloExchange.h"
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
    const peano4::CellConnectivity& cellConnectivity,
    const int                       face,
    const int                       side,
    const int                       element,
    const int                       round
  ) {
    std::int64_t key = cellConnectivity.getFaceLevel(face) * DIMENSIONS + cellConnectivity.getFaceNormal(face);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * LatticeRange + cellConnectivity.getFaceLattice(face)(axis);
    }
    return static_cast<double>((key * 2 + side) * ElementsPerFace + element + round);
  }

  /**
   * Returns for every exchanged face the side this rank owns and -1 for every other face. A face
   * kind restricts the exchanged faces to that kind.
   */
  std::vector<int> getLocalSidesOfFaces(
    const peano4::CellConnectivity&        cellConnectivity,
    const std::optional<peano4::FaceKind>& kind = std::nullopt
  ) {
    std::vector<int> sides(static_cast<std::size_t>(cellConnectivity.getNumberOfFaces()), -1);
    for (const auto& neighbour : cellConnectivity.getNeighbours()) {
      for (const auto& entry : cellConnectivity.getExchangedFaces(neighbour)) {
        if (not kind or cellConnectivity.getFaceKind(entry.face) == *kind) {
          sides[static_cast<std::size_t>(entry.face)] = entry.side;
        }
      }
    }
    return sides;
  }

  int getNumberOfExchangedFaces(
    const peano4::CellConnectivity&        cellConnectivity,
    const std::optional<peano4::FaceKind>& kind = std::nullopt
  ) {
    int faces = 0;
    for (const auto& neighbour : cellConnectivity.getNeighbours()) {
      for (const auto& entry : cellConnectivity.getExchangedFaces(neighbour)) {
        if (not kind or cellConnectivity.getFaceKind(entry.face) == *kind) {
          faces++;
        }
      }
    }
    return faces;
  }

  TARGET_DECLARE
  /**
   * Writes the marker into one face and the expected value into the half of it that this rank owns.
   */
  struct WriteLocalHalf final {
    peano4::CellConnectivity cellConnectivity{};
    Pool::View               faces{};
    const int*               sideOfFace{nullptr};
    int                      round{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      double* const chunk = faces.getChunk(face);
      for (int element = 0; element < ElementsPerFace; element++) {
        chunk[element] = Untouched;
      }
      const int side = sideOfFace[face];
      if (side >= 0) {
        for (int element = 0; element < ElementsPerHalf; element++) {
          chunk[side * ElementsPerHalf + element] = getExpectedValue(cellConnectivity, face, side, element, round);
        }
      }
    }
  };

  /**
   * Writes the number of elements of one face that differ from the expected value of their side
   * on an exchanged face and from the marker on every other face.
   */
  struct CountMismatchesOfFace final {
    peano4::CellConnectivity cellConnectivity{};
    Pool::ConstView          faces{};
    const int*               sideOfFace{nullptr};
    int                      round{0};
    int*                     mismatchesOfFace{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const double* const chunk      = faces.getChunk(face);
      const int           side       = sideOfFace[face];
      int                 mismatches = 0;
      for (int element = 0; element < ElementsPerFace; element++) {
        double expected = Untouched;
        if (side >= 0) {
          const int elementSide = element / ElementsPerHalf;
          expected =
            getExpectedValue(cellConnectivity, face, elementSide, element - elementSide * ElementsPerHalf, round);
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
    const peano4::CellConnectivity&        cellConnectivity,
    const tarch::accelerator::Buffer<int>& sides,
    const int                              round
  ) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      WriteLocalHalf{cellConnectivity.onDevice(), pool.getView(), sides.getData(), round}
    );
  }

  /**
   * Returns the number of face elements that differ from the expected value on an exchanged face or
   * from the marker elsewhere.
   */
  int getNumberOfMismatches(
    const Pool&                            pool,
    const peano4::CellConnectivity&        cellConnectivity,
    const tarch::accelerator::Buffer<int>& sides,
    const int                              round
  ) {
    const int                       numberOfFaces = cellConnectivity.getNumberOfFaces();
    tarch::accelerator::Buffer<int> mismatchesOfFace(static_cast<std::size_t>(numberOfFaces), pool.getLocation());
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(numberOfFaces),
      CountMismatchesOfFace{
        cellConnectivity.onDevice(),
        pool.getView(),
        sides.getData(),
        round,
        mismatchesOfFace.getData()
      }
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
  testMethod(testCoarseExchangeFillsTheRemoteHalvesOfCoarseFacesOnly);
  testMethod(testTwoExchangesOfOnePoolGetDistinctTags);
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
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    pool.reserve(cellConnectivity.getNumberOfFaces());
    exchange.rebuild(cellConnectivity);

    const int  numberOfExchangedFaces = getNumberOfExchangedFaces(cellConnectivity);
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
    sides.setValues(getLocalSidesOfFaces(cellConnectivity));

    for (const int round : {0, 1}) {
      writeLocalHalves(pool, cellConnectivity, sides, round);

      exchange.post();
      exchange.wait();

      validateEqualsWithParams3(getNumberOfMismatches(pool, cellConnectivity, sides, round), 0, grid, round, rank);
    }
  }
}

/**
 * An exchange of coarse faces on a grid with level jumps between ranks fills the remote half of
 * every exchanged coarse face and leaves every other face unchanged.
 */
void
peano4::tests::HaloExchangeTest::testCoarseExchangeFillsTheRemoteHalvesOfCoarseFacesOnly() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int rank          = tarch::mpi::Rank::getInstance().getRank();
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfFirstChildOfRoot(Depth)});
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

  Pool pool(ElementsPerFace, tarch::MemoryLocation::Device);
  pool.reserve(cellConnectivity.getNumberOfFaces());
  Exchange exchange(pool, peano4::FaceKind::Coarse);
  exchange.rebuild(cellConnectivity);
  const int numberOfCoarseFaces       = getNumberOfExchangedFaces(cellConnectivity, peano4::FaceKind::Coarse);
  const int globalNumberOfCoarseFaces = peano4::tests::getGlobalSumOf(numberOfCoarseFaces);
  validateEqualsWithParams1(exchange.getNumberOfEntries(), numberOfCoarseFaces, rank);
  validateEqualsWithParams1(globalNumberOfCoarseFaces > 0, numberOfRanks > 1, globalNumberOfCoarseFaces);

  tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
  sides.setValues(getLocalSidesOfFaces(cellConnectivity, peano4::FaceKind::Coarse));
  writeLocalHalves(pool, cellConnectivity, sides, 0);
  exchange.post();
  exchange.wait();
  validateEqualsWithParams1(getNumberOfMismatches(pool, cellConnectivity, sides, 0), 0, rank);
}

/**
 * Two exchanges of one pool get distinct tags in their first rebuild. Every rank reserves the same
 * tags.
 */
void
peano4::tests::HaloExchangeTest::testTwoExchangesOfOnePoolGetDistinctTags() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
  Exchange    first(pool);
  Exchange    second(pool);
  validateEquals(first.getAllocatedBytes().getTotal(), std::size_t{0});
  pool.reserve(cellConnectivity.getNumberOfFaces());
  first.rebuild(cellConnectivity);
  second.rebuild(cellConnectivity);
  validate(first.getTag() != second.getTag());
  validate(second.getTag() > first.getTag());

  const int      tags[2]{first.getTag(), second.getTag()};
  int            minimumTags[2]{0, 0};
  int            maximumTags[2]{0, 0};
  const MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(tags, minimumTags, 2, MPI_INT, MPI_MIN, communicator);
  tarch::mpi::allreduce(tags, maximumTags, 2, MPI_INT, MPI_MAX, communicator);
  validateEquals(minimumTags[0], maximumTags[0]);
  validateEquals(minimumTags[1], maximumTags[1]);
}

/**
 * The release frees the buffers and keeps the tag. The next rebuild restores a working exchange.
 */
void
peano4::tests::HaloExchangeTest::testReleaseFreesTheBuffersAndKeepsTheTag() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
  pool.reserve(cellConnectivity.getNumberOfFaces());
  Exchange exchange(pool);
  exchange.rebuild(cellConnectivity);
  const int tag             = exchange.getTag();
  const int numberOfEntries = exchange.getNumberOfEntries();

  exchange.release();
  validateEquals(exchange.getAllocatedBytes().getTotal(), std::size_t{0});
  validateEquals(exchange.getNumberOfEntries(), 0);
  validateEquals(exchange.getTag(), tag);

  exchange.rebuild(cellConnectivity);
  validateEquals(exchange.getNumberOfEntries(), numberOfEntries);
  tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
  sides.setValues(getLocalSidesOfFaces(cellConnectivity));
  writeLocalHalves(pool, cellConnectivity, sides, 0);
  exchange.post();
  exchange.wait();
  validateEquals(getNumberOfMismatches(pool, cellConnectivity, sides, 0), 0);
}
