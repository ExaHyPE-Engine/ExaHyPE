// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PartitionTest.h"

#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "peano4/topology/Tree.h"
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

  int getNumberOfFacesOfKind(const peano4::Partition& partition, const peano4::FaceKind kind) {
    return partition.getFaces(peano4::Adjacency::Local, kind).size
      + partition.getFaces(peano4::Adjacency::Remote, kind).size;
  }

  int getNumberOfExchangedFaces(const peano4::Partition& partition) {
    int faces = 0;
    for (const auto& neighbour : partition.getNeighbours()) {
      faces += partition.getExchangedFaces(neighbour).size;
    }
    return faces;
  }

  int getNumberOfLevelJumps(const peano4::Partition& partition) {
    return partition.getLevelJumps(peano4::Adjacency::Local).size
      + partition.getLevelJumps(peano4::Adjacency::Remote).size;
  }

  /**
   * Returns 1 for every face in a remote span and 0 for every other face.
   */
  std::vector<char> getRemoteFaceMarks(const peano4::Partition& partition) {
    std::vector<char> remote(static_cast<std::size_t>(partition.getNumberOfFaces()), 0);
    for (const auto kind : FaceKinds) {
      for (const int face : partition.getFaces(peano4::Adjacency::Remote, kind)) {
        remote[static_cast<std::size_t>(face)] = 1;
      }
    }
    return remote;
  }

  /**
   * Returns for every face how often it appears among the faces of the cells.
   */
  std::vector<int> getFaceReferenceCounts(const peano4::Partition& partition) {
    std::vector<int> counts(static_cast<std::size_t>(partition.getNumberOfFaces()), 0);
    for (const int cell : partition.getCells()) {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        counts[static_cast<std::size_t>(partition.getCellFace(cell, faceNumber))]++;
      }
    }
    return counts;
  }

  /**
   * Returns whether two cells, possibly on different levels, share a part of a face: Their boxes
   * touch along exactly one axis and overlap along every other axis.
   */
  bool shareAFace(const peano4::Partition& partition, const int first, const int second) {
    const int  finerLevel = std::max(partition.getCellLevel(first), partition.getCellLevel(second));
    const auto getExtent  = [&partition, finerLevel](const int cell, const int axis) {
      int scale = 1;
      for (int level = partition.getCellLevel(cell); level < finerLevel; level++) {
        scale *= peano4::SubdivisionFactor;
      }
      const int lower = partition.getCellLattice(cell)(axis) * scale;
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
    peano4::Partition partition{};
    const int*        cells{nullptr};
    double*           centres{nullptr};
    double*           widths{nullptr};
    int*              faces{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index) const {
      const int cell = cells[index];
      centres[index] = partition.getCellCentre(cell, 0);
      widths[index]  = partition.getCellWidth(cell, DIMENSIONS - 1);
      faces[index]   = partition.getCellFace(cell, 0);
    }
  };
  TARGET_DECLARE_END
} // namespace

peano4::tests::PartitionTest::PartitionTest() :
  tarch::tests::TestCase("peano4::tests::PartitionTest") {}

void
peano4::tests::PartitionTest::run() {
  testMethod(testTheRegularGridLinksEveryFaceToItsCells);
  testMethod(testThePeriodicGridHasNoBoundaryFaces);
  testMethod(testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace);
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
peano4::tests::PartitionTest::testTheRegularGridLinksEveryFaceToItsCells() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& spacetree    = peano4::Spacetree::getInstance();
  const auto& partition    = spacetree.getPartition();
  const int   cellsPerAxis = peano4::topology::getCellsPerAxis(Depth);

  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), peano4::power(cellsPerAxis, DIMENSIONS));
  validateEquals(partition.getNumberOfSlots(), partition.getCells().size);
  validateEquals(partition.getMinimumLevel(), Depth);
  validateEquals(partition.getMaximumLevel(), Depth);
  validateEquals(
    partition.getCells(peano4::Adjacency::Local).size + partition.getCells(peano4::Adjacency::Remote).size,
    partition.getCells().size
  );
  validateEquals(
    partition.getCells(peano4::Adjacency::Local, Depth).size,
    partition.getCells(peano4::Adjacency::Local).size
  );
  validate(partition.getCells(peano4::Adjacency::Local, Depth - 1).isEmpty());
  validate(partition.getCells(peano4::Adjacency::Remote, Depth + 1).isEmpty());
  validateEquals(getNumberOfLevelJumps(partition), 0);
  const bool hasNoNeighbours = partition.getNeighbours().isEmpty();
  const bool runsAlone       = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
  validateEquals(hasNoNeighbours, runsAlone);

  const int facesPerNormal = peano4::power(cellsPerAxis, DIMENSIONS - 1);
  validate(partition.getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Boundary).isEmpty());
  validateEquals(
    peano4::tests::getGlobalSumOf(getNumberOfFacesOfKind(partition, peano4::FaceKind::Boundary)),
    2 * DIMENSIONS * facesPerNormal
  );
  // An exchanged face lies in the partition of both ranks, so the sum over the ranks counts it twice.
  validateEquals(
    peano4::tests::getGlobalSumOf(getNumberOfFacesOfKind(partition, peano4::FaceKind::Interior))
      - peano4::tests::getGlobalSumOf(getNumberOfExchangedFaces(partition)) / 2,
    DIMENSIONS * facesPerNormal * (cellsPerAxis - 1)
  );
  int facesOfAllKinds = 0;
  for (const auto kind : FaceKinds) {
    facesOfAllKinds += getNumberOfFacesOfKind(partition, kind);
  }
  validateEquals(facesOfAllKinds, partition.getNumberOfFaces());

  validateNumericalEquals(partition.getWidthOfLevel(Depth, 0), 1.0 / cellsPerAxis);
  validateNumericalEquals(partition.getDomainOffset()(DIMENSIONS - 1), 0.0);
  const auto counts = getFaceReferenceCounts(partition);
  for (int face = 0; face < partition.getNumberOfFaces(); face++) {
    const bool interior = partition.getFaceKind(face) == peano4::FaceKind::Interior;
    validateWithParams1(interior or partition.getFaceKind(face) == peano4::FaceKind::Boundary, face);
    validateEqualsWithParams1(partition.getFaceLevel(face), Depth, face);
    validateNumericalEqualsWithParams1(partition.getFaceWidth(face, 0), 1.0 / cellsPerAxis, face);
    const bool exchanged          = partition.getFaceCell(face, 0) < 0 or partition.getFaceCell(face, 1) < 0;
    const int  expectedReferences = interior and not exchanged ? 2 : 1;
    validateEqualsWithParams1(counts[static_cast<std::size_t>(face)], expectedReferences, face);
  }
  for (const int cell : partition.getCells()) {
    validateEqualsWithParams1(spacetree.getCellAt(Depth, partition.getCellLattice(cell)), cell, cell);
    validateEqualsWithParams1(partition.getCellLevel(cell), Depth, cell);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateNumericalEqualsWithParams2(partition.getCellWidth(cell)(axis), 1.0 / cellsPerAxis, cell, axis);
      validateNumericalEqualsWithParams2(
        partition.getCellCentre(cell)(axis),
        (partition.getCellLattice(cell)(axis) + 0.5) / cellsPerAxis,
        cell,
        axis
      );
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int face    = partition.getCellFace(cell, faceNumber);
      const int ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      validateEqualsWithParams2(partition.getFaceCell(face, ownSide), cell, cell, faceNumber);
      validateEqualsWithParams2(partition.getFaceNormal(face), faceNumber % DIMENSIONS, cell, faceNumber);
      validateEqualsWithParams2(
        partition.getFaceLattice(face)(faceNumber % DIMENSIONS),
        partition.getCellLattice(cell)(faceNumber % DIMENSIONS) + (faceNumber < DIMENSIONS ? 0 : 1),
        cell,
        faceNumber
      );
      validateNumericalEqualsWithParams2(
        partition.getFaceCentre(face)(faceNumber % DIMENSIONS),
        static_cast<double>(partition.getFaceLattice(face)(faceNumber % DIMENSIONS)) / cellsPerAxis,
        cell,
        faceNumber
      );
    }
  }
}

void
peano4::tests::PartitionTest::testThePeriodicGridHasNoBoundaryFaces() {
  std::bitset<DIMENSIONS> periodic;
  periodic.set();
  peano4::tests::buildRegularGrid(Depth, periodic);
  const auto& partition    = peano4::Spacetree::getInstance().getPartition();
  const int   cellsPerAxis = peano4::topology::getCellsPerAxis(Depth);

  validateEquals(getNumberOfFacesOfKind(partition, peano4::FaceKind::Boundary), 0);
  validateEquals(
    peano4::tests::getGlobalSumOf(partition.getNumberOfFaces())
      - peano4::tests::getGlobalSumOf(getNumberOfExchangedFaces(partition)) / 2,
    DIMENSIONS * peano4::power(cellsPerAxis, DIMENSIONS)
  );
  for (int face = 0; face < partition.getNumberOfFaces(); face++) {
    validateWithParams1(partition.getFaceKind(face) == peano4::FaceKind::Interior, face);
    validateWithParams1(partition.getFaceCell(face, 0) >= 0 or partition.getFaceCell(face, 1) >= 0, face);
    validateWithParams1(partition.getFaceLattice(face)(partition.getFaceNormal(face)) < cellsPerAxis, face);
  }
}

/**
 * Every face between the refined centre block and a coarse cell forms one level jump, which the rank
 * of the fine cells registers.
 */
void
peano4::tests::PartitionTest::testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& partition    = peano4::Spacetree::getInstance().getPartition();
  const int   cellsPerAxis = peano4::topology::getCellsPerAxis(Depth);
  const int   blockPerAxis = peano4::tests::getNumberOfCellsPerAxisRefinedAroundTheCentre();
  const int   refinedCells = peano4::power(blockPerAxis, DIMENSIONS);
  const int   coarseCells  = peano4::power(cellsPerAxis, DIMENSIONS) - refinedCells;
  const int   fineCells    = refinedCells * peano4::ChildrenPerCell;
  const int   levelJumps   = 2 * DIMENSIONS * peano4::power(blockPerAxis, DIMENSIONS - 1);
  const auto  cellsOfLevel = [&partition](const int level) {
    return partition.getCells(peano4::Adjacency::Local, level).size
      + partition.getCells(peano4::Adjacency::Remote, level).size;
  };

  // A failed check ends the test on its rank alone, so every global sum runs before the first check.
  const int globalCoarseCells = peano4::tests::getGlobalSumOf(cellsOfLevel(Depth));
  const int globalFineCells   = peano4::tests::getGlobalSumOf(cellsOfLevel(Depth + 1));
  const int globalLevelJumps  = peano4::tests::getGlobalSumOf(getNumberOfLevelJumps(partition));
  // A rank may own cells of one of the two levels only.
  const int minimumLevel = cellsOfLevel(Depth) > 0 ? Depth : Depth + 1;
  const int maximumLevel = cellsOfLevel(Depth + 1) > 0 ? Depth + 1 : Depth;

  validateEquals(globalCoarseCells, coarseCells);
  validateEquals(globalFineCells, fineCells);
  validateEquals(globalLevelJumps, levelJumps);
  validateEquals(cellsOfLevel(Depth) + cellsOfLevel(Depth + 1), partition.getCells().size);
  validateEquals(partition.getMinimumLevel(), minimumLevel);
  validateEquals(partition.getMaximumLevel(), maximumLevel);
  validateEquals(getNumberOfFacesOfKind(partition, peano4::FaceKind::Intermediate), 0);
  validateEquals(
    getNumberOfFacesOfKind(partition, peano4::FaceKind::Hanging),
    getNumberOfLevelJumps(partition) * peano4::SubfacesPerFace
  );
  validate(getNumberOfFacesOfKind(partition, peano4::FaceKind::Coarse) >= getNumberOfLevelJumps(partition));
  for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
    validateEquals(partition.getLevelJumps(adjacency, Depth).size, partition.getLevelJumps(adjacency).size);
    validate(partition.getLevelJumps(adjacency, Depth + 1).isEmpty());
    validate(partition.getLevelJumps(adjacency, Depth - 1).isEmpty());
    for (const auto& levelJump : partition.getLevelJumps(adjacency)) {
      validateEquals(levelJump.level, Depth);
      validate(partition.getFaceKind(levelJump.coarseFace) == peano4::FaceKind::Coarse);
      validateEquals(partition.getFaceLevel(levelJump.coarseFace), Depth);
      validateEquals(partition.getFaceCell(levelJump.coarseFace, 1 - levelJump.coarseSide), -1);
      const int  coarseCell      = partition.getFaceCell(levelJump.coarseFace, levelJump.coarseSide);
      const bool coarseCellLocal = coarseCell >= 0;
      const bool jumpLocal       = adjacency == peano4::Adjacency::Local;
      validateEquals(coarseCellLocal, jumpLocal);
      if (coarseCell >= 0) {
        validateEquals(partition.getCellLevel(coarseCell), Depth);
      }
      const int normal = partition.getFaceNormal(levelJump.coarseFace);
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        const int fineFace = levelJump.fineFaces[subface];
        validateWithParams1(partition.getFaceKind(fineFace) == peano4::FaceKind::Hanging, subface);
        validateEqualsWithParams1(partition.getFaceLevel(fineFace), Depth + 1, subface);
        validateEqualsWithParams1(partition.getFaceNormal(fineFace), normal, subface);
        const int fineCell = partition.getFaceCell(fineFace, 1 - levelJump.coarseSide);
        validateWithParams1(fineCell >= 0, subface);
        validateEqualsWithParams1(partition.getCellLevel(fineCell), Depth + 1, subface);
        validateEqualsWithParams1(partition.getFaceCell(fineFace, levelJump.coarseSide), -1, subface);
        validateEqualsWithParams1(
          partition.getFaceLattice(fineFace)(normal),
          partition.getFaceLattice(levelJump.coarseFace)(normal) * peano4::SubdivisionFactor,
          subface
        );
      }
    }
  }
}

/**
 * Every span contains only entries of its adjacency and its level or kind, and the spans together
 * cover every list.
 */
void
peano4::tests::PartitionTest::testTheListsGroupByAdjacencyLevelAndKind() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& partition = peano4::Spacetree::getInstance().getPartition();
  const auto  remote    = getRemoteFaceMarks(partition);

  const auto cellHasRemoteFace = [&partition, &remote](const int cell) {
    bool hasRemoteFace = false;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      hasRemoteFace = hasRemoteFace or remote[static_cast<std::size_t>(partition.getCellFace(cell, faceNumber))] != 0;
    }
    return hasRemoteFace;
  };
  int cellsInLevelSpans = 0;
  for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
    const bool remoteSpan = adjacency == peano4::Adjacency::Remote;
    for (const int cell : partition.getCells(adjacency)) {
      const bool hasRemoteFace = cellHasRemoteFace(cell);
      validateEqualsWithParams1(hasRemoteFace, remoteSpan, cell);
    }
    for (int level = 0; level <= partition.getMaximumLevel(); level++) {
      for (const int cell : partition.getCells(adjacency, level)) {
        validateEqualsWithParams2(partition.getCellLevel(cell), level, cell, level);
      }
      cellsInLevelSpans += partition.getCells(adjacency, level).size;
    }
    for (const auto kind : FaceKinds) {
      for (const int face : partition.getFaces(adjacency, kind)) {
        validateWithParams1(partition.getFaceKind(face) == kind, face);
        const bool faceRemote = remote[static_cast<std::size_t>(face)] != 0;
        validateEqualsWithParams1(faceRemote, remoteSpan, face);
      }
    }
    validateEquals(
      partition.getFaces(adjacency, peano4::FaceKind::Interior, peano4::FaceKind::Hanging).size,
      partition.getFaces(adjacency, peano4::FaceKind::Interior).size
        + partition.getFaces(adjacency, peano4::FaceKind::Boundary).size
        + partition.getFaces(adjacency, peano4::FaceKind::Hanging).size
    );
    for (const auto& levelJump : partition.getLevelJumps(adjacency)) {
      const bool coarseFaceRemote = remote[static_cast<std::size_t>(levelJump.coarseFace)] != 0;
      validateEquals(coarseFaceRemote, remoteSpan);
      for (const int fineFace : levelJump.fineFaces) {
        const bool fineFaceRemote = remote[static_cast<std::size_t>(fineFace)] != 0;
        validateEquals(fineFaceRemote, remoteSpan);
      }
    }
  }
  validateEquals(cellsInLevelSpans, partition.getCells().size);
  // The own side of an exchanged coarse face has no cell where the fine cells below it fill that half.
  for (const auto& neighbour : partition.getNeighbours()) {
    for (const auto& entry : partition.getExchangedFaces(neighbour)) {
      const bool ownSideFilled = partition.getFaceCell(entry.face, entry.side) >= 0
        or partition.getFaceKind(entry.face) == peano4::FaceKind::Coarse;
      validateWithParams2(remote[static_cast<std::size_t>(entry.face)] != 0, neighbour.rank, entry.face);
      validateWithParams2(ownSideFilled, neighbour.rank, entry.face);
      validateWithParams2(partition.getFaceCell(entry.face, 1 - entry.side) < 0, neighbour.rank, entry.face);
    }
  }
}

/**
 * With live data, a regrid keeps the slot of every surviving cell, fills every new cell once and
 * reuses freed slots before the slot range grows.
 */
void
peano4::tests::PartitionTest::testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes() {
  peano4::tests::buildRegularGrid(Depth);
  auto&       spacetree = peano4::Spacetree::getInstance();
  const auto& partition = spacetree.getPartition();
  spacetree.markPersistentDataLive();

  const auto keyOf = [](const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::vector<int> key(DIMENSIONS, 0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key[static_cast<std::size_t>(axis)] = lattice(axis);
    }
    return key;
  };
  std::map<std::vector<int>, int> slotOfCoarseLattice;
  for (const int cell : partition.getCells()) {
    slotOfCoarseLattice[keyOf(partition.getCellLattice(cell))] = cell;
  }
  const int cellsBefore = partition.getCells().size;

  const auto checkTheMoves = [this, &partition](const peano4::Regrid& regrid, const int oldCells, const int newLevel) {
    int received = 0;
    for (const auto& rankTransfer : regrid.rankTransfers) {
      received += static_cast<int>(rankTransfer.receivedSlots.size());
    }
    const int created   = static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size()) + received;
    const int survivors = partition.getCells().size - created;
    validateEquals(static_cast<int>(regrid.freedSlots.size()), oldCells - survivors);
    validateEquals(regrid.newNumberOfSlots, partition.getNumberOfSlots());
    validate(
      static_cast<int>(regrid.createdSlots.size())
      >= static_cast<int>(regrid.prolongations.size() + regrid.restrictions.size())
    );
    for (const int slot : regrid.createdSlots) {
      validateEqualsWithParams1(partition.getCellLevel(slot), newLevel, slot);
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
  for (const int cell : partition.getCells()) {
    if (partition.getCellLevel(cell) == Depth) {
      const auto entry = slotOfCoarseLattice.find(keyOf(partition.getCellLattice(cell)));
      if (entry != slotOfCoarseLattice.end()) {
        validateEqualsWithParams1(entry->second, cell, cell);
      }
    }
  }
  const int cellsAfterRefinement = partition.getCells().size;
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
peano4::tests::PartitionTest::testTheExchangedFacesMatchAcrossRanks() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int rank          = tarch::mpi::Rank::getInstance().getRank();
  MPI_Comm  communicator  = tarch::mpi::Rank::getInstance().getCommunicator();

  for (const bool refined : {false, true}) {
    peano4::tests::buildRegularGrid(Depth);
    if (refined) {
      peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
    }
    const auto& partition = peano4::Spacetree::getInstance().getPartition();
    if (numberOfRanks == 1) {
      validate(partition.getNeighbours().isEmpty());
      continue;
    }

    for (const auto& neighbour : partition.getNeighbours()) {
      validate(neighbour.rank != rank);
      std::vector<std::int64_t> keys;
      for (const auto& entry : partition.getExchangedFaces(neighbour)) {
        keys.push_back(partition.getFaceLevel(entry.face));
        keys.push_back(partition.getFaceNormal(entry.face));
        std::int64_t lattice = 0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          lattice = lattice * 4096 + partition.getFaceLattice(entry.face)(axis);
        }
        keys.push_back(lattice);
        keys.push_back(entry.side);
      }
      int numberOfKeys = static_cast<int>(keys.size());
      int partnerKeys  = 0;
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
 * those of the lower ranks, and two cells with consecutive indices share a face. This holds on the
 * regular grid, on the refined grid and after a regrid that keeps the slots of live data.
 */
void
peano4::tests::PartitionTest::testTheCurveIndicesNumberTheCellsAlongTheCurve() {
  const int   numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  MPI_Comm    communicator  = tarch::mpi::Rank::getInstance().getCommunicator();
  auto&       spacetree     = peano4::Spacetree::getInstance();
  const auto& partition     = spacetree.getPartition();

  const auto checkTheCurveIndices = [this, numberOfRanks, communicator, &partition](const int scenario) {
    std::vector<std::pair<int, int>> cellsAlongCurve;
    for (const int cell : partition.getCells()) {
      cellsAlongCurve.emplace_back(partition.getCellCurveIndex(cell), cell);
    }
    std::sort(cellsAlongCurve.begin(), cellsAlongCurve.end());
    for (std::size_t position = 1; position < cellsAlongCurve.size(); position++) {
      const int previousIndex = cellsAlongCurve[position - 1].first;
      const int previousCell  = cellsAlongCurve[position - 1].second;
      const int index         = cellsAlongCurve[position].first;
      const int cell          = cellsAlongCurve[position].second;
      validateEqualsWithParams2(index, previousIndex + 1, scenario, cell);
      validateWithParams3(shareAFace(partition, previousCell, cell), scenario, previousCell, cell);
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
 * A kernel reading the device twin derives the same geometry and faces as the host partition.
 */
void
peano4::tests::PartitionTest::testTheDeviceTwinReadsTheSameGeometry() {
  peano4::tests::buildRegularGrid(Depth);
  peano4::tests::refineGrid({peano4::tests::getRefinementOfCentreCell(Depth)});
  const auto& partition = peano4::Spacetree::getInstance().getPartition();
  const auto  cells     = partition.getCells();

  const auto                         numberOfCells = static_cast<std::size_t>(cells.size);
  tarch::accelerator::Buffer<double> centres(numberOfCells, tarch::MemoryLocation::Device);
  tarch::accelerator::Buffer<double> widths(numberOfCells, tarch::MemoryLocation::Device);
  tarch::accelerator::Buffer<int>    faces(numberOfCells, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(cells.size),
    WriteCellGeometry{
      partition.onDevice(),
      partition.onDevice().getCells().data,
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
    validateEqualsWithParams1(centresOnHost[static_cast<std::size_t>(index)], partition.getCellCentre(cell, 0), cell);
    validateEqualsWithParams1(
      widthsOnHost[static_cast<std::size_t>(index)],
      partition.getCellWidth(cell, DIMENSIONS - 1),
      cell
    );
    validateEqualsWithParams1(facesOnHost[static_cast<std::size_t>(index)], partition.getCellFace(cell, 0), cell);
  }
  validateEquals(partition.onDevice().getNumberOfSlots(), partition.getNumberOfSlots());
  validateEquals(partition.onDevice().getNumberOfFaces(), partition.getNumberOfFaces());
  validateEquals(partition.onDevice().getMaximumLevel(), partition.getMaximumLevel());
  validateEquals(
    partition.onDevice().getCells(peano4::Adjacency::Local).size,
    partition.getCells(peano4::Adjacency::Local).size
  );
}

/**
 * The partition bytes cover the cell and face arrays on the host and, with a GPU backend, on the
 * device.
 */
void
peano4::tests::PartitionTest::testTheBytesCountEveryArray() {
  peano4::tests::buildRegularGrid(1);
  const auto& spacetree = peano4::Spacetree::getInstance();
  const auto& partition = spacetree.getPartition();

  const auto cellArrays = static_cast<std::size_t>(partition.getCells().size) * (2 + DIMENSIONS + peano4::FacesPerCell);
  const auto faceArrays = static_cast<std::size_t>(partition.getNumberOfFaces()) * (2 + DIMENSIONS + 2);
  const auto bytes      = spacetree.getStatistics().partitionBytes;
  validate(bytes.host >= (cellArrays + faceArrays) * sizeof(int));
#if defined(PEANO_WITH_GPU_BACKEND)
  validate(bytes.device >= (cellArrays + faceArrays) * sizeof(int));
#else
  validateEquals(bytes.device, std::size_t{0});
#endif
}
