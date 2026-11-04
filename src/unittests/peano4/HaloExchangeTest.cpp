// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/HaloExchange.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Spacetree.h"
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
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

#include <mpi.h>

namespace {
  TARGET_DECLARE
  constexpr int    Depth           = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;
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

UNIT_TEST("An exchange fills the remote half of every exchanged face and serves one cell connectivity after another") {
  GIVEN("A pool of faces and one exchange over the pool") {
    Pool     pool(ElementsPerFace, tarch::MemoryLocation::Device);
    Exchange exchange(pool);

    WHEN("The exchange serves a regular, a refined and a periodic grid in turn, with two rounds on every grid") {
      std::vector<int>                entriesOfGrid;
      std::vector<int>                exchangedFacesOfGrid;
      std::vector<std::size_t>        bytesOfGrid;
      std::vector<std::array<int, 2>> mismatchesOfGrid;
      for (const int grid : {0, 1, 2}) {
        const bool              refined  = grid == 1;
        const bool              periodic = grid == 2;
        std::bitset<DIMENSIONS> periodicBoundaries;
        if (periodic) {
          periodicBoundaries.set();
        }
        peano4::unittests::buildRegularGrid(Depth, periodicBoundaries);
        if (refined) {
          peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
        }
        const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

        pool.reserve(cellConnectivity.getNumberOfFaces());
        exchange.rebuild(cellConnectivity);
        entriesOfGrid.push_back(exchange.getNumberOfEntries());
        exchangedFacesOfGrid.push_back(getNumberOfExchangedFaces(cellConnectivity));
        bytesOfGrid.push_back(exchange.getAllocatedBytes().getTotal());

        tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
        sides.setValues(getLocalSidesOfFaces(cellConnectivity));

        std::array<int, 2> mismatchesOfRound{};
        for (const int round : {0, 1}) {
          writeLocalHalves(pool, cellConnectivity, sides, round);

          exchange.post();
          exchange.wait();

          mismatchesOfRound[round] = getNumberOfMismatches(pool, cellConnectivity, sides, round);
        }
        mismatchesOfGrid.push_back(mismatchesOfRound);
      }
      const bool runsAlone = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
      const int  rank      = tarch::mpi::Rank::getInstance().getRank();

      THEN("Every rebuild lists the exchanged faces and every round fills the remote halves and keeps the markers") {
        for (std::size_t grid = 0; grid < entriesOfGrid.size(); grid++) {
          VALIDATE_EQUALS(entriesOfGrid[grid], exchangedFacesOfGrid[grid], grid);
          VALIDATE_EQUALS(exchangedFacesOfGrid[grid] == 0, runsAlone, grid);
          VALIDATE(
            bytesOfGrid[grid] >= static_cast<std::size_t>(exchangedFacesOfGrid[grid]) * sizeof(peano4::ExchangedFace),
            grid
          );
          for (std::size_t round = 0; round < mismatchesOfGrid[grid].size(); round++) {
            VALIDATE_EQUALS(mismatchesOfGrid[grid][round], 0, grid, round, rank);
          }
        }
      }
    }
  }
}

UNIT_TEST("An exchange of coarse faces fills the remote halves of the exchanged coarse faces only") {
  GIVEN("A grid with level jumps between the ranks, a pool of its faces and an exchange of the coarse faces") {
    peano4::unittests::buildRegularGrid(Depth);
    peano4::unittests::refineGrid({peano4::unittests::getRefinementOfFirstChildOfRoot(Depth)});
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
    pool.reserve(cellConnectivity.getNumberOfFaces());
    Exchange exchange(pool, peano4::FaceKind::Coarse);

    WHEN("The exchange rebuilds on the grid and exchanges the halves of the coarse faces in one round") {
      exchange.rebuild(cellConnectivity);
      const int numberOfEntries           = exchange.getNumberOfEntries();
      const int numberOfCoarseFaces       = getNumberOfExchangedFaces(cellConnectivity, peano4::FaceKind::Coarse);
      const int globalNumberOfCoarseFaces = peano4::unittests::getGlobalSumOf(numberOfCoarseFaces);

      tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
      sides.setValues(getLocalSidesOfFaces(cellConnectivity, peano4::FaceKind::Coarse));
      writeLocalHalves(pool, cellConnectivity, sides, 0);
      exchange.post();
      exchange.wait();
      const int  mismatches = getNumberOfMismatches(pool, cellConnectivity, sides, 0);
      const bool runsAlone  = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
      const int  rank       = tarch::mpi::Rank::getInstance().getRank();

      THEN("The exchange lists the exchanged coarse faces, fills their remote halves and keeps the markers elsewhere") {
        VALIDATE_EQUALS(numberOfEntries, numberOfCoarseFaces, rank);
        VALIDATE_EQUALS(globalNumberOfCoarseFaces > 0, not runsAlone, globalNumberOfCoarseFaces);
        VALIDATE_EQUALS(mismatches, 0, rank);
      }
    }
  }
}

UNIT_TEST("Two exchanges of one pool get distinct tags in their first rebuild and every rank reserves the same tags") {
  GIVEN("A regular grid, a pool of its faces, two exchanges over the pool and the bytes the first one allocates") {
    peano4::unittests::buildRegularGrid(Depth);
    const auto&       cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    Pool              pool(ElementsPerFace, tarch::MemoryLocation::Device);
    Exchange          first(pool);
    Exchange          second(pool);
    const std::size_t bytesBeforeTheRebuild = first.getAllocatedBytes().getTotal();

    WHEN("Both exchanges rebuild on the grid and the ranks reduce both tags to their minimum and maximum") {
      pool.reserve(cellConnectivity.getNumberOfFaces());
      first.rebuild(cellConnectivity);
      second.rebuild(cellConnectivity);

      const int      tags[2]{first.getTag(), second.getTag()};
      int            minimumTags[2]{0, 0};
      int            maximumTags[2]{0, 0};
      const MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
      tarch::mpi::allreduce(tags, minimumTags, 2, MPI_INT, MPI_MIN, communicator);
      tarch::mpi::allreduce(tags, maximumTags, 2, MPI_INT, MPI_MAX, communicator);

      THEN("The first exchange counts zero bytes before the rebuild and every rank reserves the same two rising tags") {
        VALIDATE_EQUALS(bytesBeforeTheRebuild, std::size_t{0});
        VALIDATE(first.getTag() != second.getTag());
        VALIDATE(second.getTag() > first.getTag());
        VALIDATE_EQUALS(minimumTags[0], maximumTags[0]);
        VALIDATE_EQUALS(minimumTags[1], maximumTags[1]);
      }
    }
  }
}

UNIT_TEST("The release frees the buffers and keeps the tag and the next rebuild restores a working exchange") {
  GIVEN("An exchange rebuilt on a regular grid, its tag and its number of entries") {
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    Pool        pool(ElementsPerFace, tarch::MemoryLocation::Device);
    pool.reserve(cellConnectivity.getNumberOfFaces());
    Exchange exchange(pool);
    exchange.rebuild(cellConnectivity);
    const int tag             = exchange.getTag();
    const int numberOfEntries = exchange.getNumberOfEntries();

    WHEN("The exchange releases") {
      exchange.release();
      const std::size_t bytesAfterTheRelease   = exchange.getAllocatedBytes().getTotal();
      const int         entriesAfterTheRelease = exchange.getNumberOfEntries();
      const int         tagAfterTheRelease     = exchange.getTag();

      THEN("The exchange counts zero bytes and zero entries and keeps its tag") {
        VALIDATE_EQUALS(bytesAfterTheRelease, std::size_t{0});
        VALIDATE_EQUALS(entriesAfterTheRelease, 0);
        VALIDATE_EQUALS(tagAfterTheRelease, tag);
      }

      AND_WHEN("The exchange rebuilds on the grid and exchanges the halves of one round") {
        exchange.rebuild(cellConnectivity);
        tarch::accelerator::Buffer<int> sides(tarch::MemoryLocation::Device);
        sides.setValues(getLocalSidesOfFaces(cellConnectivity));
        writeLocalHalves(pool, cellConnectivity, sides, 0);
        exchange.post();
        exchange.wait();
        const int mismatches = getNumberOfMismatches(pool, cellConnectivity, sides, 0);

        THEN("The exchange counts its entries again and fills the remote halves") {
          VALIDATE_EQUALS(exchange.getNumberOfEntries(), numberOfEntries);
          VALIDATE_EQUALS(mismatches, 0);
        }
      }
    }
  }
}
