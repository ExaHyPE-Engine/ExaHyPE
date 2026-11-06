// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/CellConnectivity.h"

#include "peano4/Constants.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/topology/Tree.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

#include <mpi.h>

namespace {
  constexpr int Depth = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;

  constexpr std::array<peano4::FaceKind, peano4::NumberOfFaceKinds> FaceKinds{
    peano4::FaceKind::Interior,
    peano4::FaceKind::Boundary,
    peano4::FaceKind::Hanging,
    peano4::FaceKind::Coarse,
    peano4::FaceKind::Intermediate
  };

  /**
   * Stores the face keys this rank sends to one neighbour and the keys the neighbour sends back.
   */
  struct KeysOfNeighbour final {
    bool                      refined{false};
    int                       neighbourRank{-1};
    int                       partnerKeys{0};
    std::vector<std::int64_t> keys{};
    std::vector<std::int64_t> received{};
  };

  int getNumberOfFacesOfKind(const peano4::CellConnectivity& cellConnectivity, const peano4::FaceKind kind) {
    return cellConnectivity.getFaces(peano4::Adjacency::Local, kind).size
      + cellConnectivity.getFaces(peano4::Adjacency::Remote, kind).size;
  }

  int getNumberOfExchangedFaces(const peano4::CellConnectivity& cellConnectivity) {
    int faces = 0;
    for (const auto& neighbour : cellConnectivity.getNeighbours()) {
      faces += cellConnectivity.getExchangedFaces(neighbour).size;
    }
    return faces;
  }

  int getNumberOfLevelJumps(const peano4::CellConnectivity& cellConnectivity) {
    return cellConnectivity.getLevelJumps(peano4::Adjacency::Local).size
      + cellConnectivity.getLevelJumps(peano4::Adjacency::Remote).size;
  }

  /**
   * Returns 1 for every face in a remote span and 0 for every other face.
   */
  std::vector<char> getRemoteFaceMarks(const peano4::CellConnectivity& cellConnectivity) {
    std::vector<char> remote(static_cast<std::size_t>(cellConnectivity.getNumberOfFaces()), 0);
    for (const auto kind : FaceKinds) {
      for (const int face : cellConnectivity.getFaces(peano4::Adjacency::Remote, kind)) {
        remote[static_cast<std::size_t>(face)] = 1;
      }
    }
    return remote;
  }

  /**
   * Returns for every face how often it appears among the faces of the cells.
   */
  std::vector<int> getFaceReferenceCounts(const peano4::CellConnectivity& cellConnectivity) {
    std::vector<int> counts(static_cast<std::size_t>(cellConnectivity.getNumberOfFaces()), 0);
    for (const int cell : cellConnectivity.getCells()) {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        counts[static_cast<std::size_t>(cellConnectivity.getCellFace(cell, faceNumber))]++;
      }
    }
    return counts;
  }

  std::vector<int> getLatticeKeyOf(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::vector<int> key(DIMENSIONS, 0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key[static_cast<std::size_t>(axis)] = lattice(axis);
    }
    return key;
  }

  void validateTheReceivedKeys(
    const std::vector<std::int64_t>& keys,
    const std::vector<std::int64_t>& received,
    const int                        partnerKeys,
    const int                        rank,
    const int                        neighbourRank,
    const bool                       refined
  ) {
    const int numberOfKeys = static_cast<int>(keys.size());
    VALIDATE(neighbourRank != rank);
    VALIDATE_EQUALS(partnerKeys, numberOfKeys, rank, neighbourRank, refined);
    for (int entry = 0; entry + 3 < std::min(numberOfKeys, partnerKeys); entry += 4) {
      for (int component = 0; component < 3; component++) {
        VALIDATE_EQUALS(
          received[static_cast<std::size_t>(entry + component)],
          keys[static_cast<std::size_t>(entry + component)],
          rank,
          entry,
          component
        );
      }
      VALIDATE_EQUALS(
        received[static_cast<std::size_t>(entry) + 3],
        1 - keys[static_cast<std::size_t>(entry) + 3],
        rank,
        entry
      );
    }
  }

  /**
   * Checks the slot moves of a regrid with live data. The regrid frees the slot of every old cell that
   * disappears. The regrid assigns every created slot to a cell of the new level. Every prolongation
   * and restriction reads freed slots and writes a created one.
   */
  void validateTheMoves(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    const int                       oldCells,
    const int                       newLevel
  ) {
    int received = 0;
    for (const auto& rankTransfer : regrid.rankTransfers) {
      received += static_cast<int>(rankTransfer.receivedSlots.size());
    }
    const int created   = static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size()) + received;
    const int survivors = cellConnectivity.getCells().size - created;
    VALIDATE_EQUALS(static_cast<int>(regrid.freedSlots.size()), oldCells - survivors);
    VALIDATE_EQUALS(regrid.newNumberOfSlots, cellConnectivity.getNumberOfSlots());
    VALIDATE(
      static_cast<int>(regrid.createdSlots.size())
      >= static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size())
    );
    for (const int slot : regrid.createdSlots) {
      VALIDATE_EQUALS(cellConnectivity.getCellLevel(slot), newLevel, slot);
      VALIDATE(std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), slot) == regrid.freedSlots.end(), slot);
    }
    for (const auto& prolongation : regrid.prolongations) {
      VALIDATE(prolongation.parentSlot >= 0 and prolongation.parentSlot < regrid.oldNumberOfSlots);
      VALIDATE(
        std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), prolongation.parentSlot)
        != regrid.freedSlots.end()
      );
      VALIDATE(
        std::find(regrid.createdSlots.begin(), regrid.createdSlots.end(), prolongation.childSlot)
        != regrid.createdSlots.end()
      );
      VALIDATE(prolongation.childPosition >= 0 and prolongation.childPosition < peano4::ChildrenPerCell);
    }
    for (const auto& restriction : regrid.restrictions) {
      VALIDATE(
        std::find(regrid.createdSlots.begin(), regrid.createdSlots.end(), restriction.parentSlot)
        != regrid.createdSlots.end()
      );
      for (const int childSlot : restriction.childSlots) {
        VALIDATE(childSlot >= 0 and childSlot < regrid.oldNumberOfSlots);
        VALIDATE(std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), childSlot) != regrid.freedSlots.end());
      }
    }
  }

  void validateTheSlotsOfTheSurvivingCoarseCells(
    const peano4::CellConnectivity&        cellConnectivity,
    const std::map<std::vector<int>, int>& slotOfCoarseLattice
  ) {
    for (const int cell : cellConnectivity.getCells()) {
      if (cellConnectivity.getCellLevel(cell) == Depth) {
        const auto entry = slotOfCoarseLattice.find(getLatticeKeyOf(cellConnectivity.getCellLattice(cell)));
        if (entry != slotOfCoarseLattice.end()) {
          VALIDATE_EQUALS(entry->second, cell, cell);
        }
      }
    }
  }

  /**
   * Returns whether two cells, possibly on different levels, touch along a part of a face: Their
   * boxes touch along exactly one axis and overlap along every other axis.
   */
  bool areFaceNeighbours(const peano4::CellConnectivity& cellConnectivity, const int first, const int second) {
    const int  finerLevel = std::max(cellConnectivity.getCellLevel(first), cellConnectivity.getCellLevel(second));
    const auto getExtent  = [&cellConnectivity, finerLevel](const int cell, const int axis) {
      int scale = 1;
      for (int level = cellConnectivity.getCellLevel(cell); level < finerLevel; level++) {
        scale *= peano4::SubdivisionFactor;
      }
      const int lower = cellConnectivity.getCellLattice(cell)(axis) * scale;
      return std::pair{lower, lower + scale};
    };
    int touchingAxes = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const auto [firstLower, firstUpper]   = getExtent(first, axis);
      const auto [secondLower, secondUpper] = getExtent(second, axis);
      if (firstUpper == secondLower or secondUpper == firstLower) {
        touchingAxes++;
      } else if (std::min(firstUpper, secondUpper) <= std::max(firstLower, secondLower)) {
        return false;
      }
    }
    return touchingAxes == 1;
  }

  /**
   * The cell counts and the first curve indices of all ranks. cellsAfterAGap lists the cells of this
   * rank whose curve index differs from the index of their predecessor plus one.
   * cellsApartFromTheirPredecessor lists the cells of this rank whose predecessor on the curve lies
   * outside their face neighbours.
   */
  struct CurveIndices final {
    std::vector<int> cellsAfterAGap{};
    std::vector<int> cellsApartFromTheirPredecessor{};
    std::vector<int> cellsOfRank{};
    std::vector<int> firstIndexOfRank{};
  };

  /**
   * Collects the curve indices of every rank. Every rank calls this function.
   */
  CurveIndices getCurveIndices(const peano4::CellConnectivity& cellConnectivity) {
    CurveIndices                     curveIndices;
    std::vector<std::pair<int, int>> cellsAlongCurve;
    for (const int cell : cellConnectivity.getCells()) {
      cellsAlongCurve.emplace_back(cellConnectivity.getCellCurveIndex(cell), cell);
    }
    std::sort(cellsAlongCurve.begin(), cellsAlongCurve.end());
    for (std::size_t position = 1; position < cellsAlongCurve.size(); position++) {
      const auto [previousIndex, previousCell] = cellsAlongCurve[position - 1];
      const auto [index, cell]                 = cellsAlongCurve[position];
      if (index != previousIndex + 1) {
        curveIndices.cellsAfterAGap.push_back(cell);
      }
      if (not areFaceNeighbours(cellConnectivity, previousCell, cell)) {
        curveIndices.cellsApartFromTheirPredecessor.push_back(cell);
      }
    }

    const int      numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const MPI_Comm communicator  = tarch::mpi::Rank::getInstance().getCommunicator();
    const int      numberOfCells = static_cast<int>(cellsAlongCurve.size());
    const int      firstIndex    = cellsAlongCurve.empty() ? 0 : cellsAlongCurve.front().first;
    curveIndices.cellsOfRank.assign(static_cast<std::size_t>(numberOfRanks), 0);
    curveIndices.firstIndexOfRank.assign(static_cast<std::size_t>(numberOfRanks), 0);
    tarch::mpi::allgather(&numberOfCells, 1, MPI_INT, curveIndices.cellsOfRank.data(), 1, MPI_INT, communicator);
    tarch::mpi::allgather(&firstIndex, 1, MPI_INT, curveIndices.firstIndexOfRank.data(), 1, MPI_INT, communicator);
    return curveIndices;
  }

  void validateTheCurveIndices(const CurveIndices& curveIndices) {
    VALIDATE(curveIndices.cellsAfterAGap.empty(), curveIndices.cellsAfterAGap.front());
    VALIDATE(curveIndices.cellsApartFromTheirPredecessor.empty(), curveIndices.cellsApartFromTheirPredecessor.front());
    int cellsOfLowerRanks = 0;
    for (std::size_t rank = 0; rank < curveIndices.cellsOfRank.size(); rank++) {
      if (curveIndices.cellsOfRank[rank] > 0) {
        VALIDATE_EQUALS(curveIndices.firstIndexOfRank[rank], cellsOfLowerRanks, rank);
      }
      cellsOfLowerRanks += curveIndices.cellsOfRank[rank];
    }
  }

  TARGET_DECLARE
  /**
   * Writes the centre along the first axis, the width along the last axis and the first face of one cell.
   */
  struct WriteCellGeometry final {
    peano4::CellConnectivity cellConnectivity{};
    const int*               cells{nullptr};
    double*                  centres{nullptr};
    double*                  widths{nullptr};
    int*                     faces{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index) const {
      const int cell = cells[index];
      centres[index] = cellConnectivity.getCellCentre(cell, 0);
      widths[index]  = cellConnectivity.getCellWidth(cell, DIMENSIONS - 1);
      faces[index]   = cellConnectivity.getCellFace(cell, 0);
    }
  };
  TARGET_DECLARE_END
} // namespace

UNIT_TEST("On the regular grid the ranks own every cell once and every face points back to its cells") {
  GIVEN("The cells per axis and the faces per normal of the regular test grid") {
    const int cellsPerAxis   = peano4::topology::getCellsPerAxis(Depth);
    const int facesPerNormal = peano4::power(cellsPerAxis, DIMENSIONS - 1);

    WHEN("The spacetree builds the regular grid over the ranks and the ranks count its cells and faces") {
      peano4::unittests::buildRegularGrid(Depth);
      const auto& spacetree        = peano4::Spacetree::getInstance();
      const auto& cellConnectivity = spacetree.getCellConnectivity();

      const int globalCells = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
      const int globalBoundaryFaces =
        peano4::unittests::getGlobalSumOf(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Boundary));
      const int globalInteriorFaces =
        peano4::unittests::getGlobalSumOf(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Interior));
      const int globalExchangedFaces = peano4::unittests::getGlobalSumOf(getNumberOfExchangedFaces(cellConnectivity));

      const bool             hasNeighbours    = not cellConnectivity.getNeighbours().isEmpty();
      const bool             runsAlone        = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
      const std::vector<int> referencesOfFace = getFaceReferenceCounts(cellConnectivity);

      THEN("Every cell lies once on the grid level and the faces of every kind match the lattice") {
        VALIDATE_EQUALS(globalCells, peano4::power(cellsPerAxis, DIMENSIONS));
        VALIDATE_EQUALS(cellConnectivity.getNumberOfSlots(), cellConnectivity.getCells().size);
        VALIDATE_EQUALS(cellConnectivity.getMinimumLevel(), Depth);
        VALIDATE_EQUALS(cellConnectivity.getMaximumLevel(), Depth);
        VALIDATE_EQUALS(
          cellConnectivity.getCells(peano4::Adjacency::Local).size
            + cellConnectivity.getCells(peano4::Adjacency::Remote).size,
          cellConnectivity.getCells().size
        );
        VALIDATE_EQUALS(
          cellConnectivity.getCells(peano4::Adjacency::Local, Depth).size,
          cellConnectivity.getCells(peano4::Adjacency::Local).size
        );
        VALIDATE(cellConnectivity.getCells(peano4::Adjacency::Local, Depth - 1).isEmpty());
        VALIDATE(cellConnectivity.getCells(peano4::Adjacency::Remote, Depth + 1).isEmpty());
        VALIDATE_EQUALS(getNumberOfLevelJumps(cellConnectivity), 0);
        VALIDATE_EQUALS(hasNeighbours, not runsAlone);

        VALIDATE(cellConnectivity.getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Boundary).isEmpty());
        VALIDATE_EQUALS(globalBoundaryFaces, 2 * DIMENSIONS * facesPerNormal);
        // An exchanged face lies in the cell connectivity of both ranks. The sum over the ranks counts it twice.
        VALIDATE_EQUALS(
          globalInteriorFaces - globalExchangedFaces / 2,
          DIMENSIONS * facesPerNormal * (cellsPerAxis - 1)
        );
        int facesOfAllKinds = 0;
        for (const auto kind : FaceKinds) {
          facesOfAllKinds += getNumberOfFacesOfKind(cellConnectivity, kind);
        }
        VALIDATE_EQUALS(facesOfAllKinds, cellConnectivity.getNumberOfFaces());
      }

      THEN("Every cell links its faces, every face links back to its cells and the geometry follows the lattice") {
        VALIDATE_NUMERICAL_EQUALS(cellConnectivity.getWidthOfLevel(Depth, 0), 1.0 / cellsPerAxis);
        VALIDATE_NUMERICAL_EQUALS(cellConnectivity.getDomainOffset()(DIMENSIONS - 1), 0.0);
        for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
          const bool interior = cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior;
          VALIDATE(interior or cellConnectivity.getFaceKind(face) == peano4::FaceKind::Boundary, face);
          VALIDATE_EQUALS(cellConnectivity.getFaceLevel(face), Depth, face);
          VALIDATE_NUMERICAL_EQUALS(cellConnectivity.getFaceWidth(face, 0), 1.0 / cellsPerAxis, face);
          const bool exchanged = cellConnectivity.getFaceCell(face, 0) < 0 or cellConnectivity.getFaceCell(face, 1) < 0;
          const int  expectedReferences = interior and not exchanged ? 2 : 1;
          VALIDATE_EQUALS(referencesOfFace[static_cast<std::size_t>(face)], expectedReferences, face);
        }
        for (const int cell : cellConnectivity.getCells()) {
          VALIDATE_EQUALS(spacetree.getCellAt(Depth, cellConnectivity.getCellLattice(cell)), cell, cell);
          VALIDATE_EQUALS(cellConnectivity.getCellLevel(cell), Depth, cell);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_NUMERICAL_EQUALS(cellConnectivity.getCellWidth(cell)(axis), 1.0 / cellsPerAxis, cell, axis);
            VALIDATE_NUMERICAL_EQUALS(
              cellConnectivity.getCellCentre(cell)(axis),
              (cellConnectivity.getCellLattice(cell)(axis) + 0.5) / cellsPerAxis,
              cell,
              axis
            );
          }
          for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
            const int face    = cellConnectivity.getCellFace(cell, faceNumber);
            const int ownSide = faceNumber < DIMENSIONS ? 1 : 0;
            VALIDATE_EQUALS(cellConnectivity.getFaceCell(face, ownSide), cell, cell, faceNumber);
            VALIDATE_EQUALS(cellConnectivity.getFaceNormal(face), faceNumber % DIMENSIONS, cell, faceNumber);
            VALIDATE_EQUALS(
              cellConnectivity.getFaceLattice(face)(faceNumber % DIMENSIONS),
              cellConnectivity.getCellLattice(cell)(faceNumber % DIMENSIONS) + (faceNumber < DIMENSIONS ? 0 : 1),
              cell,
              faceNumber
            );
            VALIDATE_NUMERICAL_EQUALS(
              cellConnectivity.getFaceCentre(face)(faceNumber % DIMENSIONS),
              static_cast<double>(cellConnectivity.getFaceLattice(face)(faceNumber % DIMENSIONS)) / cellsPerAxis,
              cell,
              faceNumber
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("On a domain centred at the origin the centres of mirror cells and of mirror faces are negatives bitwise") {
  GIVEN("The regular grid of the test depth on the domain from -1 to 1 along every axis") {
    auto& spacetree = peano4::Spacetree::getInstance();
    spacetree.shutdown();
    spacetree.init(tarch::la::Vector<DIMENSIONS, double>(-1.0), tarch::la::Vector<DIMENSIONS, double>(2.0));
    spacetree.scheduleRegularGridConstruction(Depth);
    peano4::Regrid regrid;
    const bool     constructed      = spacetree.regrid({}, regrid);
    const auto&    cellConnectivity = spacetree.getCellConnectivity();
    const int      cellsPerAxis     = peano4::topology::getCellsPerAxis(Depth);

    WHEN("The ranks gather the centres of the cells and of the faces along every axis by their maximum") {
      // A mirror cell can lie on another rank. The maximum over the ranks gathers the centres of all
      // cells and faces of one axis.
      std::vector<std::vector<double>> centresOfCells;
      std::vector<std::vector<double>> centresOfFaces;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        std::vector<double> cellCentres(cellsPerAxis, std::numeric_limits<double>::lowest());
        std::vector<double> faceCentres(cellsPerAxis + 1, std::numeric_limits<double>::lowest());
        for (const int cell : cellConnectivity.getCells()) {
          const int index = cellConnectivity.getCellLattice(cell)(axis);
          ASSERTION(index >= 0 and index < cellsPerAxis, index, cellsPerAxis);
          cellCentres[index] = cellConnectivity.getCellCentre(cell, axis);
          faceCentres[index] = cellConnectivity.getFaceCentre(cellConnectivity.getCellFace(cell, axis), axis);
          faceCentres[index + 1] =
            cellConnectivity.getFaceCentre(cellConnectivity.getCellFace(cell, axis + DIMENSIONS), axis);
        }
        centresOfCells.push_back(peano4::unittests::getGlobalMaximaOf(cellCentres));
        centresOfFaces.push_back(peano4::unittests::getGlobalMaximaOf(faceCentres));
      }

      THEN("Every cell centre and every face centre along an axis is the negated centre of its mirror bitwise") {
        VALIDATE(constructed);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int index = 0; index < cellsPerAxis; index++) {
            VALIDATE_EQUALS(centresOfCells[axis][index], -centresOfCells[axis][cellsPerAxis - 1 - index], axis, index);
          }
          for (int index = 0; index <= cellsPerAxis; index++) {
            VALIDATE_EQUALS(centresOfFaces[axis][index], -centresOfFaces[axis][cellsPerAxis - index], axis, index);
          }
        }
      }
    }
  }
}

UNIT_TEST("Every face of the periodic grid is interior") {
  GIVEN("Periodic boundaries along every axis and the cells per axis of the test grid") {
    std::bitset<DIMENSIONS> periodic;
    periodic.set();
    const int cellsPerAxis = peano4::topology::getCellsPerAxis(Depth);

    WHEN("The spacetree builds the periodic grid over the ranks and the ranks count its faces") {
      peano4::unittests::buildRegularGrid(Depth, periodic);
      const auto& cellConnectivity     = peano4::Spacetree::getInstance().getCellConnectivity();
      const int   globalFaces          = peano4::unittests::getGlobalSumOf(cellConnectivity.getNumberOfFaces());
      const int   globalExchangedFaces = peano4::unittests::getGlobalSumOf(getNumberOfExchangedFaces(cellConnectivity));

      THEN("Every face is an interior face with a cell below the upper wall and the faces match the lattice") {
        VALIDATE_EQUALS(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Boundary), 0);
        VALIDATE_EQUALS(globalFaces - globalExchangedFaces / 2, DIMENSIONS * peano4::power(cellsPerAxis, DIMENSIONS));
        for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
          VALIDATE(cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior, face);
          VALIDATE(cellConnectivity.getFaceCell(face, 0) >= 0 or cellConnectivity.getFaceCell(face, 1) >= 0, face);
          VALIDATE(cellConnectivity.getFaceLattice(face)(cellConnectivity.getFaceNormal(face)) < cellsPerAxis, face);
        }
      }
    }
  }
}

UNIT_TEST("Every face between the centre block and a coarse cell is one level jump of the rank of the fine cells") {
  GIVEN("The cells of the test grid, of the block refined around its centre and the level jumps around the block") {
    const int cellsPerAxis = peano4::topology::getCellsPerAxis(Depth);
    const int blockPerAxis = peano4::unittests::getNumberOfCellsPerAxisRefinedAroundTheCentre();
    const int refinedCells = peano4::power(blockPerAxis, DIMENSIONS);
    const int coarseCells  = peano4::power(cellsPerAxis, DIMENSIONS) - refinedCells;
    const int fineCells    = refinedCells * peano4::ChildrenPerCell;
    const int levelJumps   = 2 * DIMENSIONS * peano4::power(blockPerAxis, DIMENSIONS - 1);

    WHEN("The spacetree builds the grid and refines the cell in the centre and the ranks count cells and jumps") {
      peano4::unittests::buildRegularGrid(Depth);
      peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
      const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
      const auto  cellsOfLevel     = [&cellConnectivity](const int level) {
        return cellConnectivity.getCells(peano4::Adjacency::Local, level).size
          + cellConnectivity.getCells(peano4::Adjacency::Remote, level).size;
      };

      const int globalCoarseCells = peano4::unittests::getGlobalSumOf(cellsOfLevel(Depth));
      const int globalFineCells   = peano4::unittests::getGlobalSumOf(cellsOfLevel(Depth + 1));
      const int globalLevelJumps  = peano4::unittests::getGlobalSumOf(getNumberOfLevelJumps(cellConnectivity));
      // A rank may own cells of one of the two levels only.
      const int minimumLevel = cellsOfLevel(Depth) > 0 ? Depth : Depth + 1;
      const int maximumLevel = cellsOfLevel(Depth + 1) > 0 ? Depth + 1 : Depth;

      THEN("The ranks own the cells and the level jumps of the refined grid and every jump links its hanging faces") {
        VALIDATE_EQUALS(globalCoarseCells, coarseCells);
        VALIDATE_EQUALS(globalFineCells, fineCells);
        VALIDATE_EQUALS(globalLevelJumps, levelJumps);
        VALIDATE_EQUALS(cellsOfLevel(Depth) + cellsOfLevel(Depth + 1), cellConnectivity.getCells().size);
        VALIDATE_EQUALS(cellConnectivity.getMinimumLevel(), minimumLevel);
        VALIDATE_EQUALS(cellConnectivity.getMaximumLevel(), maximumLevel);
        VALIDATE_EQUALS(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Intermediate), 0);
        VALIDATE_EQUALS(
          getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Hanging),
          getNumberOfLevelJumps(cellConnectivity) * peano4::SubfacesPerFace
        );
        VALIDATE(
          getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Coarse) >= getNumberOfLevelJumps(cellConnectivity)
        );
      }

      THEN("Every level jump links a coarse face of the grid level to the hanging faces of the fine cells beside it") {
        for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
          VALIDATE_EQUALS(
            cellConnectivity.getLevelJumps(adjacency, Depth).size,
            cellConnectivity.getLevelJumps(adjacency).size
          );
          VALIDATE(cellConnectivity.getLevelJumps(adjacency, Depth + 1).isEmpty());
          VALIDATE(cellConnectivity.getLevelJumps(adjacency, Depth - 1).isEmpty());
          VALIDATE_EQUALS(
            cellConnectivity.getLevelJumps(adjacency, 0, Depth + 1).size,
            cellConnectivity.getLevelJumps(adjacency).size
          );
          VALIDATE(
            cellConnectivity.getLevelJumps(adjacency, 0, Depth + 1).data
            == cellConnectivity.getLevelJumps(adjacency).data
          );
          VALIDATE(cellConnectivity.getLevelJumps(adjacency, Depth + 1, Depth + 2).isEmpty());
          for (const auto& levelJump : cellConnectivity.getLevelJumps(adjacency)) {
            VALIDATE_EQUALS(levelJump.level, Depth);
            VALIDATE(cellConnectivity.getFaceKind(levelJump.coarseFace) == peano4::FaceKind::Coarse);
            VALIDATE_EQUALS(cellConnectivity.getFaceLevel(levelJump.coarseFace), Depth);
            VALIDATE_EQUALS(cellConnectivity.getFaceCell(levelJump.coarseFace, 1 - levelJump.coarseSide), -1);
            const int  coarseCell      = cellConnectivity.getFaceCell(levelJump.coarseFace, levelJump.coarseSide);
            const bool coarseCellLocal = coarseCell >= 0;
            const bool jumpLocal       = adjacency == peano4::Adjacency::Local;
            VALIDATE_EQUALS(coarseCellLocal, jumpLocal);
            if (coarseCell >= 0) {
              VALIDATE_EQUALS(cellConnectivity.getCellLevel(coarseCell), Depth);
            }
            const int normal = cellConnectivity.getFaceNormal(levelJump.coarseFace);
            for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
              const int fineFace = levelJump.fineFaces[subface];
              VALIDATE(cellConnectivity.getFaceKind(fineFace) == peano4::FaceKind::Hanging, subface);
              VALIDATE_EQUALS(cellConnectivity.getFaceLevel(fineFace), Depth + 1, subface);
              VALIDATE_EQUALS(cellConnectivity.getFaceNormal(fineFace), normal, subface);
              const int fineCell = cellConnectivity.getFaceCell(fineFace, 1 - levelJump.coarseSide);
              VALIDATE(fineCell >= 0, subface);
              VALIDATE_EQUALS(cellConnectivity.getCellLevel(fineCell), Depth + 1, subface);
              VALIDATE_EQUALS(cellConnectivity.getFaceCell(fineFace, levelJump.coarseSide), -1, subface);
              VALIDATE_EQUALS(
                cellConnectivity.getFaceLattice(fineFace)(normal),
                cellConnectivity.getFaceLattice(levelJump.coarseFace)(normal) * peano4::SubdivisionFactor,
                subface
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("On a grid with level jumps the cell centre beside every face returns the bits of the centre of its cell") {
  GIVEN("The regular test grid") {
    peano4::unittests::buildRegularGrid(Depth);

    WHEN("The cell in the centre refines") {
      peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
      const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

      THEN("The centre beside every side of a face with a cell has the bits of the centre of that cell") {
        for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
          for (int side = 0; side < 2; side++) {
            const int cell = cellConnectivity.getFaceCell(face, side);
            if (cell >= 0) {
              const tarch::la::Vector<DIMENSIONS, double> centre = cellConnectivity.getCellCentreBesideFace(face, side);
              for (int axis = 0; axis < DIMENSIONS; axis++) {
                VALIDATE_EQUALS(centre(axis), cellConnectivity.getCellCentre(cell, axis), face, side, axis);
              }
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Every span contains only entries of its adjacency and its level or kind and the spans cover every list") {
  GIVEN("The test grid with the block refined around its centre") {
    peano4::unittests::buildRegularGrid(Depth);
    peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    WHEN("Every rank marks the faces of its remote spans") {
      const auto remote            = getRemoteFaceMarks(cellConnectivity);
      const auto cellHasRemoteFace = [&cellConnectivity, &remote](const int cell) {
        bool hasRemoteFace = false;
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          hasRemoteFace =
            hasRemoteFace or remote[static_cast<std::size_t>(cellConnectivity.getCellFace(cell, faceNumber))] != 0;
        }
        return hasRemoteFace;
      };

      THEN("A span lists only entries of its adjacency and its level or kind and the level spans list every cell") {
        int cellsInLevelSpans = 0;
        for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
          const bool remoteSpan = adjacency == peano4::Adjacency::Remote;
          for (const int cell : cellConnectivity.getCells(adjacency)) {
            const bool hasRemoteFace = cellHasRemoteFace(cell);
            VALIDATE_EQUALS(hasRemoteFace, remoteSpan, cell);
          }
          for (int level = 0; level <= cellConnectivity.getMaximumLevel(); level++) {
            for (const int cell : cellConnectivity.getCells(adjacency, level)) {
              VALIDATE_EQUALS(cellConnectivity.getCellLevel(cell), level, cell, level);
            }
            cellsInLevelSpans += cellConnectivity.getCells(adjacency, level).size;
          }
          for (const auto kind : FaceKinds) {
            for (const int face : cellConnectivity.getFaces(adjacency, kind)) {
              VALIDATE(cellConnectivity.getFaceKind(face) == kind, face);
              const bool faceRemote = remote[static_cast<std::size_t>(face)] != 0;
              VALIDATE_EQUALS(faceRemote, remoteSpan, face);
            }
          }
          VALIDATE_EQUALS(
            cellConnectivity.getFaces(adjacency, peano4::FaceKind::Interior, peano4::FaceKind::Hanging).size,
            cellConnectivity.getFaces(adjacency, peano4::FaceKind::Interior).size
              + cellConnectivity.getFaces(adjacency, peano4::FaceKind::Boundary).size
              + cellConnectivity.getFaces(adjacency, peano4::FaceKind::Hanging).size
          );
          for (const auto& levelJump : cellConnectivity.getLevelJumps(adjacency)) {
            const bool coarseFaceRemote = remote[static_cast<std::size_t>(levelJump.coarseFace)] != 0;
            VALIDATE_EQUALS(coarseFaceRemote, remoteSpan);
            for (const int fineFace : levelJump.fineFaces) {
              const bool fineFaceRemote = remote[static_cast<std::size_t>(fineFace)] != 0;
              VALIDATE_EQUALS(fineFaceRemote, remoteSpan);
            }
          }
        }
        VALIDATE_EQUALS(cellsInLevelSpans, cellConnectivity.getCells().size);
      }

      THEN("Every exchanged face is remote and is either coarse or touches a cell on its own side") {
        for (const auto& neighbour : cellConnectivity.getNeighbours()) {
          for (const auto& entry : cellConnectivity.getExchangedFaces(neighbour)) {
            const bool ownSideFilled = cellConnectivity.getFaceCell(entry.face, entry.side) >= 0
              or cellConnectivity.getFaceKind(entry.face) == peano4::FaceKind::Coarse;
            VALIDATE(remote[static_cast<std::size_t>(entry.face)] != 0, neighbour.rank, entry.face);
            VALIDATE(ownSideFilled, neighbour.rank, entry.face);
            VALIDATE(cellConnectivity.getFaceCell(entry.face, 1 - entry.side) < 0, neighbour.rank, entry.face);
          }
        }
      }
    }
  }
}

UNIT_TEST("A regrid with live data keeps the slots of surviving cells and fills freed slots before the range grows") {
  GIVEN("The test grid with live data, the slot of every coarse cell by its lattice position and the cell count") {
    peano4::unittests::buildRegularGrid(Depth);
    auto&       spacetree        = peano4::Spacetree::getInstance();
    const auto& cellConnectivity = spacetree.getCellConnectivity();
    spacetree.markPersistentDataLive();

    std::map<std::vector<int>, int> slotOfCoarseLattice;
    for (const int cell : cellConnectivity.getCells()) {
      slotOfCoarseLattice[getLatticeKeyOf(cellConnectivity.getCellLattice(cell))] = cell;
    }
    const int cellsBefore = cellConnectivity.getCells().size;

    WHEN("The cell in the centre refines") {
      peano4::Regrid refine;
      const bool     refined = spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, refine);
      const int      cellsAfterRefinement = cellConnectivity.getCells().size;
      spacetree.finishRegrid();

      THEN("Every fine cell receives one move and every surviving coarse cell keeps its slot") {
        VALIDATE(refined);
        validateTheMoves(cellConnectivity, refine, cellsBefore, Depth + 1);
        validateTheSlotsOfTheSurvivingCoarseCells(cellConnectivity, slotOfCoarseLattice);
        VALIDATE_EQUALS(refine.oldNumberOfSlots, cellsBefore);
        VALIDATE(refine.restrictions.empty());
      }

      AND_WHEN("Every cell erases back") {
        peano4::Regrid erase;
        const bool     erased = spacetree.regrid(peano4::unittests::getErasureOfEveryCell(Depth), erase);
        spacetree.finishRegrid();

        THEN("Every new coarse cell receives one move and the freed slots take the coarse cells without growth") {
          VALIDATE(erased);
          validateTheMoves(cellConnectivity, erase, cellsAfterRefinement, Depth);
          VALIDATE_EQUALS(erase.oldNumberOfSlots, refine.newNumberOfSlots);
          VALIDATE_EQUALS(erase.newNumberOfSlots, erase.oldNumberOfSlots);
          VALIDATE(erase.prolongations.empty());
        }

        AND_WHEN("The cell in the centre refines again") {
          peano4::Regrid refineAgain;
          const bool     refinedAgain =
            spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, refineAgain);
          spacetree.finishRegrid();

          THEN("The freed slots take the fine cells without growth") {
            VALIDATE(refinedAgain);
            VALIDATE_EQUALS(refineAgain.newNumberOfSlots, refineAgain.oldNumberOfSlots);
          }
        }
      }
    }
  }
}

UNIT_TEST("Every rank pair derives the same sequence of exchanged faces, each rank from its own side") {
  GIVEN("The rank, the number of ranks and the communicator") {
    const int      numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int      rank          = tarch::mpi::Rank::getInstance().getRank();
    const MPI_Comm communicator  = tarch::mpi::Rank::getInstance().getCommunicator();

    WHEN("Every rank sends the keys of its exchanged faces to each neighbour, on the regular and on the refined grid") {
      std::vector<int>             numbersOfNeighbours;
      std::vector<KeysOfNeighbour> exchanges;
      for (const bool refined : {false, true}) {
        peano4::unittests::buildRegularGrid(Depth);
        if (refined) {
          peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
        }
        const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
        numbersOfNeighbours.push_back(cellConnectivity.getNeighbours().size);
        if (numberOfRanks == 1) {
          continue;
        }

        for (const auto& neighbour : cellConnectivity.getNeighbours()) {
          std::vector<std::int64_t> keys;
          for (const auto& entry : cellConnectivity.getExchangedFaces(neighbour)) {
            keys.push_back(cellConnectivity.getFaceLevel(entry.face));
            keys.push_back(cellConnectivity.getFaceNormal(entry.face));
            std::int64_t lattice = 0;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              lattice = lattice * 4096 + cellConnectivity.getFaceLattice(entry.face)(axis);
            }
            keys.push_back(lattice);
            keys.push_back(entry.side);
          }
          const int numberOfKeys = static_cast<int>(keys.size());
          int       partnerKeys  = 0;
          tarch::mpi::sendrecv(
            &numberOfKeys,
            1,
            MPI_INT,
            neighbour.rank,
            0,
            &partnerKeys,
            1,
            MPI_INT,
            neighbour.rank,
            0,
            communicator
          );
          std::vector<std::int64_t> received(static_cast<std::size_t>(std::max(partnerKeys, 1)));
          tarch::mpi::sendrecv(
            keys.data(),
            numberOfKeys,
            MPI_INT64_T,
            neighbour.rank,
            1,
            received.data(),
            partnerKeys,
            MPI_INT64_T,
            neighbour.rank,
            1,
            communicator
          );
          exchanges.push_back({refined, neighbour.rank, partnerKeys, std::move(keys), std::move(received)});
        }
      }

      THEN("A single rank finds zero neighbours and every neighbour sends the keys of this rank with the other side") {
        if (numberOfRanks == 1) {
          for (const int numberOfNeighbours : numbersOfNeighbours) {
            VALIDATE_EQUALS(numberOfNeighbours, 0);
          }
        }
        for (const auto& exchange : exchanges) {
          validateTheReceivedKeys(
            exchange.keys,
            exchange.received,
            exchange.partnerKeys,
            rank,
            exchange.neighbourRank,
            exchange.refined
          );
        }
      }
    }
  }
}

UNIT_TEST("The curve indices number every cell once rank after rank and consecutive cells are face neighbours") {
  GIVEN("The spacetree of this process and its cell connectivity") {
    auto&       spacetree        = peano4::Spacetree::getInstance();
    const auto& cellConnectivity = spacetree.getCellConnectivity();

    WHEN("The spacetree builds the regular test grid and refines around its centre cell without live data") {
      peano4::unittests::buildRegularGrid(Depth);
      const CurveIndices regularGrid = getCurveIndices(cellConnectivity);
      peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
      const CurveIndices refinedGrid = getCurveIndices(cellConnectivity);

      THEN("The curve indices of both grids number the cells along the curve") {
        validateTheCurveIndices(regularGrid);
        validateTheCurveIndices(refinedGrid);
      }
    }

    WHEN("The spacetree builds the regular test grid with live data and refines around its centre cell") {
      peano4::unittests::buildRegularGrid(Depth);
      spacetree.markPersistentDataLive();
      peano4::Regrid regrid;
      const bool     changesTheGrid   = spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, regrid);
      const CurveIndices curveIndices = getCurveIndices(cellConnectivity);
      spacetree.finishRegrid();

      THEN("The regrid changes the grid and the curve indices number the cells along the curve") {
        VALIDATE(changesTheGrid);
        validateTheCurveIndices(curveIndices);
      }
    }
  }
}

UNIT_TEST("A kernel reading the device twin of the cell connectivity derives the geometry and the faces of the host") {
  GIVEN("The test grid with the block refined around its centre") {
    peano4::unittests::buildRegularGrid(Depth);
    peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells            = cellConnectivity.getCells();

    WHEN("A kernel writes the centre, the width and the first face of every cell from the device twin") {
      const auto                         numberOfCells = static_cast<std::size_t>(cells.size);
      tarch::accelerator::Buffer<double> centres(numberOfCells, tarch::MemoryLocation::Device);
      tarch::accelerator::Buffer<double> widths(numberOfCells, tarch::MemoryLocation::Device);
      tarch::accelerator::Buffer<int>    faces(numberOfCells, tarch::MemoryLocation::Device);
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(cells.size),
        WriteCellGeometry{
          cellConnectivity.onDevice(),
          cellConnectivity.onDevice().getCells().data,
          centres.getData(),
          widths.getData(),
          faces.getData()
        }
      );
      const auto centresOnHost = centres.getValues();
      const auto widthsOnHost  = widths.getValues();
      const auto facesOnHost   = faces.getValues();

      THEN("Every value equals its host value and the getters of the twin return the values of the host") {
        for (int index = 0; index < cells.size; index++) {
          const int cell = cells[index];
          VALIDATE_EQUALS(
            centresOnHost[static_cast<std::size_t>(index)],
            cellConnectivity.getCellCentre(cell, 0),
            cell
          );
          VALIDATE_EQUALS(
            widthsOnHost[static_cast<std::size_t>(index)],
            cellConnectivity.getCellWidth(cell, DIMENSIONS - 1),
            cell
          );
          VALIDATE_EQUALS(facesOnHost[static_cast<std::size_t>(index)], cellConnectivity.getCellFace(cell, 0), cell);
        }
        VALIDATE_EQUALS(cellConnectivity.onDevice().getNumberOfSlots(), cellConnectivity.getNumberOfSlots());
        VALIDATE_EQUALS(cellConnectivity.onDevice().getNumberOfFaces(), cellConnectivity.getNumberOfFaces());
        VALIDATE_EQUALS(cellConnectivity.onDevice().getMaximumLevel(), cellConnectivity.getMaximumLevel());
        VALIDATE_EQUALS(
          cellConnectivity.onDevice().getCells(peano4::Adjacency::Local).size,
          cellConnectivity.getCells(peano4::Adjacency::Local).size
        );
      }
    }
  }
}

UNIT_TEST(
  "A built cell connectivity stores its cell and face arrays on the host and, with a GPU backend, on the device"
) {
  GIVEN("The depth one") {
    constexpr int ShallowDepth = 1;

    WHEN("The spacetree builds the regular grid of this depth over the ranks") {
      peano4::unittests::buildRegularGrid(ShallowDepth);
      const auto& spacetree        = peano4::Spacetree::getInstance();
      const auto& cellConnectivity = spacetree.getCellConnectivity();

      const auto cellArrays =
        static_cast<std::size_t>(cellConnectivity.getCells().size) * (2 + DIMENSIONS + peano4::FacesPerCell);
      const auto faceArrays = static_cast<std::size_t>(cellConnectivity.getNumberOfFaces()) * (2 + DIMENSIONS + 2);
      const auto bytes      = spacetree.getStatistics().cellConnectivityBytes;

      THEN("The statistics count every cell and face array on the host and with a GPU backend on the device") {
        VALIDATE(bytes.host >= (cellArrays + faceArrays) * sizeof(int));
#if defined(PEANO_WITH_GPU_BACKEND)
        VALIDATE(bytes.device >= (cellArrays + faceArrays) * sizeof(int));
#else
        VALIDATE_EQUALS(bytes.device, std::size_t{0});
#endif
      }
    }
  }
}
