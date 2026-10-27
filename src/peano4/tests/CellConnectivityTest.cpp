// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellConnectivityTest.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
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
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

namespace {
  constexpr int Depth = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;

  constexpr std::array<peano4::FaceKind, peano4::NumberOfFaceKinds> FaceKinds{
    peano4::FaceKind::Interior,
    peano4::FaceKind::Boundary,
    peano4::FaceKind::Hanging,
    peano4::FaceKind::Coarse,
    peano4::FaceKind::Intermediate
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

  /**
   * Returns whether two cells, possibly on different levels, touch along a part of a face: Their
   * boxes touch along exactly one axis and overlap along every other axis.
   */
  bool touchAlongAFace(const peano4::CellConnectivity& cellConnectivity, const int first, const int second) {
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

peano4::tests::CellConnectivityTest::CellConnectivityTest() :
  tarch::tests::TestCase("peano4::tests::CellConnectivityTest") {}

void
peano4::tests::CellConnectivityTest::run() {
  testMethod(testTheRegularGridLinksEveryFaceToItsCells);
  testMethod(testTheCentresOfMirrorCellsAndFacesAreNegated);
  testMethod(testEveryFaceOfThePeriodicGridIsInterior);
  testMethod(testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace);
  testMethod(testTheCellCentreBesideEveryFaceMatchesItsCellBitwise);
  testMethod(testTheListsGroupByAdjacencyLevelAndKind);
  testMethod(testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes);
  testMethod(testTheExchangedFacesMatchAcrossRanks);
  testMethod(testTheCurveIndicesNumberTheCellsAlongTheCurve);
  testMethod(testTheDeviceTwinReadsTheSameGeometry);
  testMethod(testTheBytesCountEveryArray);
}

/**
 * On the regular grid, the ranks together own every cell once, every face points back to its cells,
 * and the geometry follows from the level and the lattice position.
 */
void
peano4::tests::CellConnectivityTest::testTheRegularGridLinksEveryFaceToItsCells() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& spacetree        = peano4::Spacetree::getInstance();
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  const int   cellsPerAxis     = peano4::topology::getCellsPerAxis(Depth);

  validateEquals(
    peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size),
    peano4::power(cellsPerAxis, DIMENSIONS)
  );
  validateEquals(cellConnectivity.getNumberOfSlots(), cellConnectivity.getCells().size);
  validateEquals(cellConnectivity.getMinimumLevel(), Depth);
  validateEquals(cellConnectivity.getMaximumLevel(), Depth);
  validateEquals(
    cellConnectivity.getCells(peano4::Adjacency::Local).size
      + cellConnectivity.getCells(peano4::Adjacency::Remote).size,
    cellConnectivity.getCells().size
  );
  validateEquals(
    cellConnectivity.getCells(peano4::Adjacency::Local, Depth).size,
    cellConnectivity.getCells(peano4::Adjacency::Local).size
  );
  validate(cellConnectivity.getCells(peano4::Adjacency::Local, Depth - 1).isEmpty());
  validate(cellConnectivity.getCells(peano4::Adjacency::Remote, Depth + 1).isEmpty());
  validateEquals(getNumberOfLevelJumps(cellConnectivity), 0);
  const bool hasNoNeighbours = cellConnectivity.getNeighbours().isEmpty();
  const bool runsAlone       = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
  validateEquals(hasNoNeighbours, runsAlone);

  const int facesPerNormal = peano4::power(cellsPerAxis, DIMENSIONS - 1);
  validate(cellConnectivity.getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Boundary).isEmpty());
  validateEquals(
    peano4::tests::getGlobalSumOf(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Boundary)),
    2 * DIMENSIONS * facesPerNormal
  );
  // An exchanged face lies in the cell connectivity of both ranks. The sum over the ranks counts it twice.
  validateEquals(
    peano4::tests::getGlobalSumOf(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Interior))
      - peano4::tests::getGlobalSumOf(getNumberOfExchangedFaces(cellConnectivity)) / 2,
    DIMENSIONS * facesPerNormal * (cellsPerAxis - 1)
  );
  int facesOfAllKinds = 0;
  for (const auto kind : FaceKinds) {
    facesOfAllKinds += getNumberOfFacesOfKind(cellConnectivity, kind);
  }
  validateEquals(facesOfAllKinds, cellConnectivity.getNumberOfFaces());

  validateNumericalEquals(cellConnectivity.getWidthOfLevel(Depth, 0), 1.0 / cellsPerAxis);
  validateNumericalEquals(cellConnectivity.getDomainOffset()(DIMENSIONS - 1), 0.0);
  const auto counts = getFaceReferenceCounts(cellConnectivity);
  for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
    const bool interior = cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior;
    validateWithParams1(interior or cellConnectivity.getFaceKind(face) == peano4::FaceKind::Boundary, face);
    validateEqualsWithParams1(cellConnectivity.getFaceLevel(face), Depth, face);
    validateNumericalEqualsWithParams1(cellConnectivity.getFaceWidth(face, 0), 1.0 / cellsPerAxis, face);
    const bool exchanged = cellConnectivity.getFaceCell(face, 0) < 0 or cellConnectivity.getFaceCell(face, 1) < 0;
    const int  expectedReferences = interior and not exchanged ? 2 : 1;
    validateEqualsWithParams1(counts[static_cast<std::size_t>(face)], expectedReferences, face);
  }
  for (const int cell : cellConnectivity.getCells()) {
    validateEqualsWithParams1(spacetree.getCellAt(Depth, cellConnectivity.getCellLattice(cell)), cell, cell);
    validateEqualsWithParams1(cellConnectivity.getCellLevel(cell), Depth, cell);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateNumericalEqualsWithParams2(cellConnectivity.getCellWidth(cell)(axis), 1.0 / cellsPerAxis, cell, axis);
      validateNumericalEqualsWithParams2(
        cellConnectivity.getCellCentre(cell)(axis),
        (cellConnectivity.getCellLattice(cell)(axis) + 0.5) / cellsPerAxis,
        cell,
        axis
      );
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int face    = cellConnectivity.getCellFace(cell, faceNumber);
      const int ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      validateEqualsWithParams2(cellConnectivity.getFaceCell(face, ownSide), cell, cell, faceNumber);
      validateEqualsWithParams2(cellConnectivity.getFaceNormal(face), faceNumber % DIMENSIONS, cell, faceNumber);
      validateEqualsWithParams2(
        cellConnectivity.getFaceLattice(face)(faceNumber % DIMENSIONS),
        cellConnectivity.getCellLattice(cell)(faceNumber % DIMENSIONS) + (faceNumber < DIMENSIONS ? 0 : 1),
        cell,
        faceNumber
      );
      validateNumericalEqualsWithParams2(
        cellConnectivity.getFaceCentre(face)(faceNumber % DIMENSIONS),
        static_cast<double>(cellConnectivity.getFaceLattice(face)(faceNumber % DIMENSIONS)) / cellsPerAxis,
        cell,
        faceNumber
      );
    }
  }
}

/**
 * On a domain centred at the origin, the centres of two cells that mirror along an axis are exact
 * negatives along that axis, and so are the centres of their two mirror faces.
 */
void
peano4::tests::CellConnectivityTest::testTheCentresOfMirrorCellsAndFacesAreNegated() {
  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.shutdown();
  spacetree.init(tarch::la::Vector<DIMENSIONS, double>(-1.0), tarch::la::Vector<DIMENSIONS, double>(2.0));
  spacetree.scheduleRegularGridConstruction(Depth);
  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({}, regrid));
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  const int   cellsPerAxis     = peano4::topology::getCellsPerAxis(Depth);

  // A mirror cell can lie on another rank. The maximum over the ranks gathers the centres of all
  // cells and faces of one axis.
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    std::vector<double> centresOfCells(cellsPerAxis, std::numeric_limits<double>::lowest());
    std::vector<double> centresOfFaces(cellsPerAxis + 1, std::numeric_limits<double>::lowest());
    for (const int cell : cellConnectivity.getCells()) {
      const int index = cellConnectivity.getCellLattice(cell)(axis);
      assertion(index >= 0 and index < cellsPerAxis, index, cellsPerAxis);
      centresOfCells[index] = cellConnectivity.getCellCentre(cell, axis);
      centresOfFaces[index] = cellConnectivity.getFaceCentre(cellConnectivity.getCellFace(cell, axis), axis);
      centresOfFaces[index + 1] =
        cellConnectivity.getFaceCentre(cellConnectivity.getCellFace(cell, axis + DIMENSIONS), axis);
    }
    centresOfCells = peano4::tests::getGlobalMaximaOf(centresOfCells);
    centresOfFaces = peano4::tests::getGlobalMaximaOf(centresOfFaces);
    for (int index = 0; index < cellsPerAxis; index++) {
      validateEqualsWithParams2(centresOfCells[index], -centresOfCells[cellsPerAxis - 1 - index], axis, index);
    }
    for (int index = 0; index <= cellsPerAxis; index++) {
      validateEqualsWithParams2(centresOfFaces[index], -centresOfFaces[cellsPerAxis - index], axis, index);
    }
  }
}

void
peano4::tests::CellConnectivityTest::testEveryFaceOfThePeriodicGridIsInterior() {
  std::bitset<DIMENSIONS> periodic;
  periodic.set();
  peano4::tests::buildRegularGrid(Depth, periodic);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const int   cellsPerAxis     = peano4::topology::getCellsPerAxis(Depth);

  validateEquals(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Boundary), 0);
  validateEquals(
    peano4::tests::getGlobalSumOf(cellConnectivity.getNumberOfFaces())
      - peano4::tests::getGlobalSumOf(getNumberOfExchangedFaces(cellConnectivity)) / 2,
    DIMENSIONS * peano4::power(cellsPerAxis, DIMENSIONS)
  );
  for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
    validateWithParams1(cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior, face);
    validateWithParams1(cellConnectivity.getFaceCell(face, 0) >= 0 or cellConnectivity.getFaceCell(face, 1) >= 0, face);
    validateWithParams1(
      cellConnectivity.getFaceLattice(face)(cellConnectivity.getFaceNormal(face)) < cellsPerAxis,
      face
    );
  }
}

/**
 * Every face between the block of the centre refinement and a coarse cell is one level jump of the
 * rank that owns the fine cells.
 */
void
peano4::tests::CellConnectivityTest::testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const int   cellsPerAxis     = peano4::topology::getCellsPerAxis(Depth);
  const int   blockPerAxis     = peano4::tests::getNumberOfCellsPerAxisRefinedAroundTheCentre();
  const int   refinedCells     = peano4::power(blockPerAxis, DIMENSIONS);
  const int   coarseCells      = peano4::power(cellsPerAxis, DIMENSIONS) - refinedCells;
  const int   fineCells        = refinedCells * peano4::ChildrenPerCell;
  const int   levelJumps       = 2 * DIMENSIONS * peano4::power(blockPerAxis, DIMENSIONS - 1);
  const auto  cellsOfLevel     = [&cellConnectivity](const int level) {
    return cellConnectivity.getCells(peano4::Adjacency::Local, level).size
      + cellConnectivity.getCells(peano4::Adjacency::Remote, level).size;
  };

  // A failed check ends the test on its rank alone. Every global sum runs before the first check.
  const int globalCoarseCells = peano4::tests::getGlobalSumOf(cellsOfLevel(Depth));
  const int globalFineCells   = peano4::tests::getGlobalSumOf(cellsOfLevel(Depth + 1));
  const int globalLevelJumps  = peano4::tests::getGlobalSumOf(getNumberOfLevelJumps(cellConnectivity));
  // A rank may own cells of one of the two levels only.
  const int minimumLevel = cellsOfLevel(Depth) > 0 ? Depth : Depth + 1;
  const int maximumLevel = cellsOfLevel(Depth + 1) > 0 ? Depth + 1 : Depth;

  validateEquals(globalCoarseCells, coarseCells);
  validateEquals(globalFineCells, fineCells);
  validateEquals(globalLevelJumps, levelJumps);
  validateEquals(cellsOfLevel(Depth) + cellsOfLevel(Depth + 1), cellConnectivity.getCells().size);
  validateEquals(cellConnectivity.getMinimumLevel(), minimumLevel);
  validateEquals(cellConnectivity.getMaximumLevel(), maximumLevel);
  validateEquals(getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Intermediate), 0);
  validateEquals(
    getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Hanging),
    getNumberOfLevelJumps(cellConnectivity) * peano4::SubfacesPerFace
  );
  validate(
    getNumberOfFacesOfKind(cellConnectivity, peano4::FaceKind::Coarse) >= getNumberOfLevelJumps(cellConnectivity)
  );
  for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
    validateEquals(
      cellConnectivity.getLevelJumps(adjacency, Depth).size,
      cellConnectivity.getLevelJumps(adjacency).size
    );
    validate(cellConnectivity.getLevelJumps(adjacency, Depth + 1).isEmpty());
    validate(cellConnectivity.getLevelJumps(adjacency, Depth - 1).isEmpty());
    validateEquals(
      cellConnectivity.getLevelJumps(adjacency, 0, Depth + 1).size,
      cellConnectivity.getLevelJumps(adjacency).size
    );
    validate(
      cellConnectivity.getLevelJumps(adjacency, 0, Depth + 1).data == cellConnectivity.getLevelJumps(adjacency).data
    );
    validate(cellConnectivity.getLevelJumps(adjacency, Depth + 1, Depth + 2).isEmpty());
    for (const auto& levelJump : cellConnectivity.getLevelJumps(adjacency)) {
      validateEquals(levelJump.level, Depth);
      validate(cellConnectivity.getFaceKind(levelJump.coarseFace) == peano4::FaceKind::Coarse);
      validateEquals(cellConnectivity.getFaceLevel(levelJump.coarseFace), Depth);
      validateEquals(cellConnectivity.getFaceCell(levelJump.coarseFace, 1 - levelJump.coarseSide), -1);
      const int  coarseCell      = cellConnectivity.getFaceCell(levelJump.coarseFace, levelJump.coarseSide);
      const bool coarseCellLocal = coarseCell >= 0;
      const bool jumpLocal       = adjacency == peano4::Adjacency::Local;
      validateEquals(coarseCellLocal, jumpLocal);
      if (coarseCell >= 0) {
        validateEquals(cellConnectivity.getCellLevel(coarseCell), Depth);
      }
      const int normal = cellConnectivity.getFaceNormal(levelJump.coarseFace);
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        const int fineFace = levelJump.fineFaces[subface];
        validateWithParams1(cellConnectivity.getFaceKind(fineFace) == peano4::FaceKind::Hanging, subface);
        validateEqualsWithParams1(cellConnectivity.getFaceLevel(fineFace), Depth + 1, subface);
        validateEqualsWithParams1(cellConnectivity.getFaceNormal(fineFace), normal, subface);
        const int fineCell = cellConnectivity.getFaceCell(fineFace, 1 - levelJump.coarseSide);
        validateWithParams1(fineCell >= 0, subface);
        validateEqualsWithParams1(cellConnectivity.getCellLevel(fineCell), Depth + 1, subface);
        validateEqualsWithParams1(cellConnectivity.getFaceCell(fineFace, levelJump.coarseSide), -1, subface);
        validateEqualsWithParams1(
          cellConnectivity.getFaceLattice(fineFace)(normal),
          cellConnectivity.getFaceLattice(levelJump.coarseFace)(normal) * peano4::SubdivisionFactor,
          subface
        );
      }
    }
  }
}

/**
 * On a grid with level jumps, the cell centre beside a face returns the bits of getCellCentre() of
 * the cell on that side.
 */
void
peano4::tests::CellConnectivityTest::testTheCellCentreBesideEveryFaceMatchesItsCellBitwise() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

  for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
    for (int side = 0; side < 2; side++) {
      const int cell = cellConnectivity.getFaceCell(face, side);
      if (cell >= 0) {
        const tarch::la::Vector<DIMENSIONS, double> centre = cellConnectivity.getCellCentreBesideFace(face, side);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          validateEqualsWithParams3(centre(axis), cellConnectivity.getCellCentre(cell, axis), face, side, axis);
        }
      }
    }
  }
}

/**
 * Every span contains only entries of its adjacency and its level or kind. The spans together
 * cover every list.
 */
void
peano4::tests::CellConnectivityTest::testTheListsGroupByAdjacencyLevelAndKind() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto  remote           = getRemoteFaceMarks(cellConnectivity);

  const auto cellHasRemoteFace = [&cellConnectivity, &remote](const int cell) {
    bool hasRemoteFace = false;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      hasRemoteFace =
        hasRemoteFace or remote[static_cast<std::size_t>(cellConnectivity.getCellFace(cell, faceNumber))] != 0;
    }
    return hasRemoteFace;
  };
  int cellsInLevelSpans = 0;
  for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
    const bool remoteSpan = adjacency == peano4::Adjacency::Remote;
    for (const int cell : cellConnectivity.getCells(adjacency)) {
      const bool hasRemoteFace = cellHasRemoteFace(cell);
      validateEqualsWithParams1(hasRemoteFace, remoteSpan, cell);
    }
    for (int level = 0; level <= cellConnectivity.getMaximumLevel(); level++) {
      for (const int cell : cellConnectivity.getCells(adjacency, level)) {
        validateEqualsWithParams2(cellConnectivity.getCellLevel(cell), level, cell, level);
      }
      cellsInLevelSpans += cellConnectivity.getCells(adjacency, level).size;
    }
    for (const auto kind : FaceKinds) {
      for (const int face : cellConnectivity.getFaces(adjacency, kind)) {
        validateWithParams1(cellConnectivity.getFaceKind(face) == kind, face);
        const bool faceRemote = remote[static_cast<std::size_t>(face)] != 0;
        validateEqualsWithParams1(faceRemote, remoteSpan, face);
      }
    }
    validateEquals(
      cellConnectivity.getFaces(adjacency, peano4::FaceKind::Interior, peano4::FaceKind::Hanging).size,
      cellConnectivity.getFaces(adjacency, peano4::FaceKind::Interior).size
        + cellConnectivity.getFaces(adjacency, peano4::FaceKind::Boundary).size
        + cellConnectivity.getFaces(adjacency, peano4::FaceKind::Hanging).size
    );
    for (const auto& levelJump : cellConnectivity.getLevelJumps(adjacency)) {
      const bool coarseFaceRemote = remote[static_cast<std::size_t>(levelJump.coarseFace)] != 0;
      validateEquals(coarseFaceRemote, remoteSpan);
      for (const int fineFace : levelJump.fineFaces) {
        const bool fineFaceRemote = remote[static_cast<std::size_t>(fineFace)] != 0;
        validateEquals(fineFaceRemote, remoteSpan);
      }
    }
  }
  validateEquals(cellsInLevelSpans, cellConnectivity.getCells().size);
  // An exchanged face is a coarse face or touches a cell on its own side.
  for (const auto& neighbour : cellConnectivity.getNeighbours()) {
    for (const auto& entry : cellConnectivity.getExchangedFaces(neighbour)) {
      const bool ownSideFilled = cellConnectivity.getFaceCell(entry.face, entry.side) >= 0
        or cellConnectivity.getFaceKind(entry.face) == peano4::FaceKind::Coarse;
      validateWithParams2(remote[static_cast<std::size_t>(entry.face)] != 0, neighbour.rank, entry.face);
      validateWithParams2(ownSideFilled, neighbour.rank, entry.face);
      validateWithParams2(cellConnectivity.getFaceCell(entry.face, 1 - entry.side) < 0, neighbour.rank, entry.face);
    }
  }
}

/**
 * With live data, a regrid keeps the slot of every surviving cell, fills every new cell once and
 * reuses freed slots before the slot range grows.
 */
void
peano4::tests::CellConnectivityTest::testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes() {
  peano4::tests::buildRegularGrid(Depth);
  auto&       spacetree        = peano4::Spacetree::getInstance();
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  spacetree.markPersistentDataLive();

  const auto keyOf = [](const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::vector<int> key(DIMENSIONS, 0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key[static_cast<std::size_t>(axis)] = lattice(axis);
    }
    return key;
  };
  std::map<std::vector<int>, int> slotOfCoarseLattice;
  for (const int cell : cellConnectivity.getCells()) {
    slotOfCoarseLattice[keyOf(cellConnectivity.getCellLattice(cell))] = cell;
  }
  const int cellsBefore = cellConnectivity.getCells().size;

  const auto checkTheMoves = [this,
                              &cellConnectivity](const peano4::Regrid& regrid, const int oldCells, const int newLevel) {
    int received = 0;
    for (const auto& rankTransfer : regrid.rankTransfers) {
      received += static_cast<int>(rankTransfer.receivedSlots.size());
    }
    const int created   = static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size()) + received;
    const int survivors = cellConnectivity.getCells().size - created;
    validateEquals(static_cast<int>(regrid.freedSlots.size()), oldCells - survivors);
    validateEquals(regrid.newNumberOfSlots, cellConnectivity.getNumberOfSlots());
    validate(
      static_cast<int>(regrid.createdSlots.size())
      >= static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size())
    );
    for (const int slot : regrid.createdSlots) {
      validateEqualsWithParams1(cellConnectivity.getCellLevel(slot), newLevel, slot);
      validateWithParams1(
        std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), slot) == regrid.freedSlots.end(),
        slot
      );
    }
    for (const auto& prolongation : regrid.prolongations) {
      validate(prolongation.parentSlot >= 0 and prolongation.parentSlot < regrid.oldNumberOfSlots);
      validate(
        std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), prolongation.parentSlot)
        != regrid.freedSlots.end()
      );
      validate(
        std::find(regrid.createdSlots.begin(), regrid.createdSlots.end(), prolongation.childSlot)
        != regrid.createdSlots.end()
      );
      validate(prolongation.childPosition >= 0 and prolongation.childPosition < peano4::ChildrenPerCell);
    }
    for (const auto& restriction : regrid.restrictions) {
      validate(
        std::find(regrid.createdSlots.begin(), regrid.createdSlots.end(), restriction.parentSlot)
        != regrid.createdSlots.end()
      );
      for (const int childSlot : restriction.childSlots) {
        validate(childSlot >= 0 and childSlot < regrid.oldNumberOfSlots);
        validate(std::find(regrid.freedSlots.begin(), regrid.freedSlots.end(), childSlot) != regrid.freedSlots.end());
      }
    }
  };

  peano4::Regrid refine;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, refine));
  validateEquals(refine.oldNumberOfSlots, cellsBefore);
  validate(refine.restrictions.empty());
  checkTheMoves(refine, cellsBefore, Depth + 1);
  for (const int cell : cellConnectivity.getCells()) {
    if (cellConnectivity.getCellLevel(cell) == Depth) {
      const auto entry = slotOfCoarseLattice.find(keyOf(cellConnectivity.getCellLattice(cell)));
      if (entry != slotOfCoarseLattice.end()) {
        validateEqualsWithParams1(entry->second, cell, cell);
      }
    }
  }
  const int cellsAfterRefinement = cellConnectivity.getCells().size;
  spacetree.finishRegrid();

  peano4::Regrid erase;
  validateTrue(spacetree.regrid(peano4::tests::getErasureOfEveryCell(Depth), erase));
  validateEquals(erase.oldNumberOfSlots, refine.newNumberOfSlots);
  validateEquals(erase.newNumberOfSlots, erase.oldNumberOfSlots);
  validate(erase.prolongations.empty());
  checkTheMoves(erase, cellsAfterRefinement, Depth);
  spacetree.finishRegrid();

  peano4::Regrid refineAgain;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, refineAgain));
  validateEquals(refineAgain.newNumberOfSlots, refineAgain.oldNumberOfSlots);
  spacetree.finishRegrid();
}

/**
 * Every rank pair derives the same sequence of exchanged faces, each from its own side, on the
 * regular and on the refined grid.
 */
void
peano4::tests::CellConnectivityTest::testTheExchangedFacesMatchAcrossRanks() {
  const int      numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int      rank          = tarch::mpi::Rank::getInstance().getRank();
  const MPI_Comm communicator  = tarch::mpi::Rank::getInstance().getCommunicator();

  for (const bool refined : {false, true}) {
    peano4::tests::buildRegularGrid(Depth);
    if (refined) {
      peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
    }
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    if (numberOfRanks == 1) {
      validate(cellConnectivity.getNeighbours().isEmpty());
      continue;
    }

    for (const auto& neighbour : cellConnectivity.getNeighbours()) {
      validate(neighbour.rank != rank);
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
      validateEqualsWithParams3(partnerKeys, numberOfKeys, rank, neighbour.rank, refined);
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
      for (int entry = 0; entry + 3 < std::min(numberOfKeys, partnerKeys); entry += 4) {
        for (int component = 0; component < 3; component++) {
          validateEqualsWithParams3(
            received[static_cast<std::size_t>(entry + component)],
            keys[static_cast<std::size_t>(entry + component)],
            rank,
            entry,
            component
          );
        }
        validateEqualsWithParams2(
          received[static_cast<std::size_t>(entry) + 3],
          1 - keys[static_cast<std::size_t>(entry) + 3],
          rank,
          entry
        );
      }
    }
  }
}

/**
 * The curve indices number the cells of all ranks once each, those of a rank consecutively behind
 * those of the lower ranks. Two cells with consecutive indices touch along a face, on the regular
 * grid, on the refined grid and after a regrid that keeps the slots of live data.
 */
void
peano4::tests::CellConnectivityTest::testTheCurveIndicesNumberTheCellsAlongTheCurve() {
  const int      numberOfRanks    = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const MPI_Comm communicator     = tarch::mpi::Rank::getInstance().getCommunicator();
  auto&          spacetree        = peano4::Spacetree::getInstance();
  const auto&    cellConnectivity = spacetree.getCellConnectivity();

  const auto checkTheCurveIndices = [this, numberOfRanks, communicator, &cellConnectivity](const int scenario) {
    std::vector<std::pair<int, int>> cellsAlongCurve;
    for (const int cell : cellConnectivity.getCells()) {
      cellsAlongCurve.emplace_back(cellConnectivity.getCellCurveIndex(cell), cell);
    }
    std::sort(cellsAlongCurve.begin(), cellsAlongCurve.end());
    for (std::size_t position = 1; position < cellsAlongCurve.size(); position++) {
      const int previousIndex = cellsAlongCurve[position - 1].first;
      const int previousCell  = cellsAlongCurve[position - 1].second;
      const int index         = cellsAlongCurve[position].first;
      const int cell          = cellsAlongCurve[position].second;
      validateEqualsWithParams2(index, previousIndex + 1, scenario, cell);
      validateWithParams3(touchAlongAFace(cellConnectivity, previousCell, cell), scenario, previousCell, cell);
    }

    const int        numberOfCells = static_cast<int>(cellsAlongCurve.size());
    const int        firstIndex    = cellsAlongCurve.empty() ? 0 : cellsAlongCurve.front().first;
    std::vector<int> cellsOfRank(static_cast<std::size_t>(numberOfRanks), 0);
    std::vector<int> firstIndexOfRank(static_cast<std::size_t>(numberOfRanks), 0);
    tarch::mpi::allgather(&numberOfCells, 1, MPI_INT, cellsOfRank.data(), 1, MPI_INT, communicator);
    tarch::mpi::allgather(&firstIndex, 1, MPI_INT, firstIndexOfRank.data(), 1, MPI_INT, communicator);
    int cellsOfLowerRanks = 0;
    for (std::size_t otherRank = 0; otherRank < cellsOfRank.size(); otherRank++) {
      if (cellsOfRank[otherRank] > 0) {
        validateEqualsWithParams2(firstIndexOfRank[otherRank], cellsOfLowerRanks, scenario, otherRank);
      }
      cellsOfLowerRanks += cellsOfRank[otherRank];
    }
  };

  peano4::tests::buildRegularGrid(Depth);
  checkTheCurveIndices(0);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  checkTheCurveIndices(1);

  peano4::tests::buildRegularGrid(Depth);
  spacetree.markPersistentDataLive();
  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid));
  checkTheCurveIndices(2);
  spacetree.finishRegrid();
}

/**
 * A kernel reading the device twin derives the same geometry and faces as the host cell connectivity.
 */
void
peano4::tests::CellConnectivityTest::testTheDeviceTwinReadsTheSameGeometry() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
  const auto  cells            = cellConnectivity.getCells();

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
  for (int index = 0; index < cells.size; index++) {
    const int cell = cells[index];
    validateEqualsWithParams1(
      centresOnHost[static_cast<std::size_t>(index)],
      cellConnectivity.getCellCentre(cell, 0),
      cell
    );
    validateEqualsWithParams1(
      widthsOnHost[static_cast<std::size_t>(index)],
      cellConnectivity.getCellWidth(cell, DIMENSIONS - 1),
      cell
    );
    validateEqualsWithParams1(
      facesOnHost[static_cast<std::size_t>(index)],
      cellConnectivity.getCellFace(cell, 0),
      cell
    );
  }
  validateEquals(cellConnectivity.onDevice().getNumberOfSlots(), cellConnectivity.getNumberOfSlots());
  validateEquals(cellConnectivity.onDevice().getNumberOfFaces(), cellConnectivity.getNumberOfFaces());
  validateEquals(cellConnectivity.onDevice().getMaximumLevel(), cellConnectivity.getMaximumLevel());
  validateEquals(
    cellConnectivity.onDevice().getCells(peano4::Adjacency::Local).size,
    cellConnectivity.getCells(peano4::Adjacency::Local).size
  );
}

/**
 * The cell connectivity bytes cover the cell and face arrays on the host and, with a GPU backend, on
 * the device.
 */
void
peano4::tests::CellConnectivityTest::testTheBytesCountEveryArray() {
  peano4::tests::buildRegularGrid(1);
  const auto& spacetree        = peano4::Spacetree::getInstance();
  const auto& cellConnectivity = spacetree.getCellConnectivity();

  const auto cellArrays =
    static_cast<std::size_t>(cellConnectivity.getCells().size) * (2 + DIMENSIONS + peano4::FacesPerCell);
  const auto faceArrays = static_cast<std::size_t>(cellConnectivity.getNumberOfFaces()) * (2 + DIMENSIONS + 2);
  const auto bytes      = spacetree.getStatistics().cellConnectivityBytes;
  validate(bytes.host >= (cellArrays + faceArrays) * sizeof(int));
#if defined(PEANO_WITH_GPU_BACKEND)
  validate(bytes.device >= (cellArrays + faceArrays) * sizeof(int));
#else
  validateEquals(bytes.device, std::size_t{0});
#endif
}
