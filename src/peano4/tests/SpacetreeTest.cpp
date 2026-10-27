// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeTest.h"

#include "peano4/CellConnectivity.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "peano4/topology/Tree.h"
#include "tarch/la/Vector.h"

namespace {
  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);

  std::int64_t getNumberOfOwnershipUnitsOnLevel(const int level) {
    std::int64_t ownershipUnits = peano4::ChildrenPerCell;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      ownershipUnits *= peano4::topology::getCellsPerAxis(level);
    }
    return ownershipUnits;
  }

  int getNumberOfLocalCellsOfLevel(const peano4::CellConnectivity& cellConnectivity, const int level) {
    return cellConnectivity.getCells(peano4::Adjacency::Local, level).size
      + cellConnectivity.getCells(peano4::Adjacency::Remote, level).size;
  }

  int getNumberOfReceivedSlots(const peano4::Regrid& regrid) {
    int received = 0;
    for (const auto& rankTransfer : regrid.rankTransfers) {
      received += static_cast<int>(rankTransfer.receivedSlots.size());
    }
    return received;
  }
} // namespace

peano4::tests::SpacetreeTest::SpacetreeTest() :
  TestCase("peano4::tests::SpacetreeTest") {}

void
peano4::tests::SpacetreeTest::run() {
  testMethod(testTheSpawnLevelOffersEnoughOwnershipUnits);
  testMethod(testTheSpawnLevelStaysAboveTheCells);
  testMethod(testShallowGridsUseTheRootSpawnLevel);
  testMethod(testTheSpawnLevelGrowsWithTheRankCount);
  testMethod(testDeepGridsKeepTheirSpawnLevelInRange);
  testMethod(testNoStatisticsStandBeforeTheFirstRegrid);
  testMethod(testARankWithoutCellsReportsNoLevel);
  testMethod(testRejectedRequestsChangeNoPlan);
  testMethod(testARegridWithLiveDataReturnsTheSlotMoves);
  testMethod(testTheTreeBytesCountTheSpareTree);
  testMethod(testTheCellConnectivityBytesCountTheFreedSlots);
  testMethod(testTheTreeBytesCountTheEvaluatedPlan);
  testMethod(testTheCellLookupFindsEveryCellAndNothingElse);
  testMethod(testThePeriodicBoundariesReturnTheInitialisedValue);
}

/**
 * requestsChangePlan() returns true only for a request set that changes the plan.
 */
void
peano4::tests::SpacetreeTest::testRejectedRequestsChangeNoPlan() {
  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.shutdown();
  spacetree.init(DomainOffset, DomainWidth);

  peano4::RefinementRequest eraseRoot;
  eraseRoot.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
  eraseRoot.level = 0;
  peano4::RefinementRequest refineCell;
  refineCell.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  refineCell.level = 1;

  validateFalse(spacetree.requestsChangePlan({}));
  validateTrue(spacetree.requestsChangePlan({eraseRoot}));

  spacetree.scheduleRegularGridConstruction(1);
  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({}, regrid));
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  const auto& statistics       = spacetree.getStatistics();
  validateEquals(peano4::tests::getGlobalSumOf(statistics.numberOfCells), peano4::ChildrenPerCell);
  validateEquals(peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size), peano4::ChildrenPerCell);
  validateEquals(peano4::tests::getGlobalSumOf(statistics.numberOfRefinedCells), 1);
  validateEquals(statistics.numberOfFaces, cellConnectivity.getNumberOfFaces());
  validateEquals(statistics.numberOfHangingFaces, 0);
  validateEquals(statistics.maximumLevel, 1);
  validateEquals(statistics.minimumLevel, 1);
  validateEquals(statistics.numberOfStationaryRegrids, 0);
  validate(statistics.cellConnectivityBytes.host > 0);
  validate(statistics.treeBytes > 0);

  validateFalse(spacetree.requestsChangePlan({}));
  validateFalse(spacetree.requestsChangePlan({eraseRoot}));
  validateFalse(spacetree.requestsChangePlan({eraseRoot}));
  validateTrue(spacetree.requestsChangePlan({refineCell}));
  validateTrue(spacetree.requestsChangePlan({eraseRoot, refineCell}));
  validateFalse(spacetree.requestsChangePlan({eraseRoot}));

  validateFalse(spacetree.regrid({eraseRoot}, regrid));
  validateEquals(statistics.numberOfStationaryRegrids, 1);
  validateFalse(spacetree.regrid({}, regrid));
  validateEquals(statistics.numberOfStationaryRegrids, 2);
}

/**
 * A regrid with live data keeps the slots of the surviving cells and fills every new cell by one
 * prolongation, one restriction or one receive.
 */
void
peano4::tests::SpacetreeTest::testARegridWithLiveDataReturnsTheSlotMoves() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  auto&       spacetree        = peano4::Spacetree::getInstance();
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  const auto& statistics       = spacetree.getStatistics();
  const int   cellsBefore      = peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size);
  validateEquals(cellsBefore, peano4::ChildrenPerCell);

  spacetree.markPersistentDataLive();

  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid));
  const int fineCells = peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth + 1));
  validate(fineCells > 0);
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.prolongations.size()) + getNumberOfReceivedSlots(regrid)),
    fineCells
  );
  validate(regrid.restrictions.empty());
  const int coarseCellsAfterRefinement =
    peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth));
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.freedSlots.size())),
    cellsBefore - coarseCellsAfterRefinement
  );
  validateEquals(
    peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size),
    coarseCellsAfterRefinement + fineCells
  );
  validateEquals(statistics.maximumLevel, Depth + 1);
  spacetree.finishRegrid();

  validateTrue(spacetree.regrid(peano4::tests::getErasureOfEveryCell(Depth), regrid));
  const int coarseCellsAfterErase =
    peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth));
  validate(coarseCellsAfterErase > coarseCellsAfterRefinement);
  validate(regrid.prolongations.empty());
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.restrictions.size()) + getNumberOfReceivedSlots(regrid)),
    coarseCellsAfterErase - coarseCellsAfterRefinement
  );
  validateEquals(
    peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size),
    coarseCellsAfterErase + peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth + 1))
  );
  spacetree.finishRegrid();
}

/**
 * After a refinement and the erase back the tree bytes count the standing tree and the spare tree
 * that the next regrid reuses.
 */
void
peano4::tests::SpacetreeTest::testTheTreeBytesCountTheSpareTree() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  auto&             spacetree      = peano4::Spacetree::getInstance();
  const std::size_t bytesOfOneTree = spacetree.getStatistics().treeBytes;
  validate(bytesOfOneTree > sizeof(peano4::topology::Tree));
  spacetree.markPersistentDataLive();

  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid));
  spacetree.finishRegrid();
  validateTrue(spacetree.regrid(peano4::tests::getErasureOfEveryCell(Depth), regrid));
  spacetree.finishRegrid();
  validateWithParams2(
    spacetree.getStatistics().treeBytes > 3 * bytesOfOneTree / 2,
    spacetree.getStatistics().treeBytes,
    bytesOfOneTree
  );
}

/**
 * finishRegrid() adds the growth of the free list after an erase to the cell connectivity bytes.
 */
void
peano4::tests::SpacetreeTest::testTheCellConnectivityBytesCountTheFreedSlots() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  auto&       spacetree  = peano4::Spacetree::getInstance();
  const auto& statistics = spacetree.getStatistics();
  spacetree.markPersistentDataLive();

  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid));
  spacetree.finishRegrid();
  validateTrue(spacetree.regrid(peano4::tests::getErasureOfEveryCell(Depth), regrid));
  const std::size_t bytesBeforeFinish = statistics.cellConnectivityBytes.host;
  spacetree.finishRegrid();
  validateWithParams2(
    statistics.cellConnectivityBytes.host >= bytesBeforeFinish,
    statistics.cellConnectivityBytes.host,
    bytesBeforeFinish
  );
  const int grownBytes = static_cast<int>(statistics.cellConnectivityBytes.host - bytesBeforeFinish);
  validateWithParams1(peano4::tests::getGlobalSumOf(grownBytes) > 0, grownBytes);
}

/**
 * The tree bytes count the plan that requestsChangePlan() evaluates and keeps for the next regrid.
 */
void
peano4::tests::SpacetreeTest::testTheTreeBytesCountTheEvaluatedPlan() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  auto&             spacetree   = peano4::Spacetree::getInstance();
  const std::size_t bytesBefore = spacetree.getStatistics().treeBytes;
  validateTrue(spacetree.requestsChangePlan({peano4::tests::getRefinementOfCentreCell(Depth)}));
  validateWithParams2(
    spacetree.getStatistics().treeBytes > bytesBefore,
    spacetree.getStatistics().treeBytes,
    bytesBefore
  );
}

/**
 * The spawn level deepens until its ownership units cover the ranks or the grid runs out of depth.
 */
void
peano4::tests::SpacetreeTest::testTheSpawnLevelOffersEnoughOwnershipUnits() {
  const int unitsOnRootLevel  = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(0));
  const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
  const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
  const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

  for (int targetLevel = 1; targetLevel <= 8; targetLevel++) {
    // The rank counts lie just below, at and just above the ownership units of a level.
    for (const int numberOfRanks :
         {1,
          2,
          unitsOnRootLevel - 1,
          unitsOnLevelOne,
          unitsOnLevelOne + 1,
          unitsOnLevelTwo,
          unitsOnLevelTwo + 1,
          unitsOnLevelThree}) {
      const auto spawnLevel         = peano4::Spacetree::deriveSpawnLevel(targetLevel, numberOfRanks);
      const bool coversEveryRank    = getNumberOfOwnershipUnitsOnLevel(spawnLevel) >= numberOfRanks;
      const bool exhaustedGridDepth = spawnLevel == std::max(targetLevel - 1, 0);
      validateWithParams2(coversEveryRank or exhaustedGridDepth, targetLevel, numberOfRanks);
    }
  }
}

void
peano4::tests::SpacetreeTest::testTheSpawnLevelStaysAboveTheCells() {
  for (int targetLevel = 0; targetLevel <= 10; targetLevel++) {
    for (const int numberOfRanks : {1, 4, 100, 10000}) {
      const auto spawnLevel = peano4::Spacetree::deriveSpawnLevel(targetLevel, numberOfRanks);
      validateWithParams2(spawnLevel >= 0, targetLevel, numberOfRanks);
      validateWithParams3(spawnLevel <= std::max(targetLevel - 1, 0), spawnLevel, targetLevel, numberOfRanks);
    }
  }
}

/**
 * A grid with no complete block level below the root uses the root as its spawn level.
 */
void
peano4::tests::SpacetreeTest::testShallowGridsUseTheRootSpawnLevel() {
  for (const int numberOfRanks : {1, 2, 1000}) {
    validateEquals(peano4::Spacetree::deriveSpawnLevel(0, numberOfRanks), 0);
    validateEquals(peano4::Spacetree::deriveSpawnLevel(1, numberOfRanks), 0);
  }
}

/**
 * The spawn level increases monotonically with the number of ranks.
 */
void
peano4::tests::SpacetreeTest::testTheSpawnLevelGrowsWithTheRankCount() {
  constexpr int DeepEnough = 9;

  const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
  const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
  const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

  int previousSpawnLevel = peano4::Spacetree::deriveSpawnLevel(DeepEnough, 1);
  validateEquals(previousSpawnLevel, 0);

  // The rank counts lie at the ownership units of a level and one rank above them.
  for (const int numberOfRanks :
       {2,
        unitsOnLevelOne,
        unitsOnLevelOne + 1,
        unitsOnLevelTwo,
        unitsOnLevelTwo + 1,
        unitsOnLevelThree,
        unitsOnLevelThree + 1}) {
    const auto spawnLevel = peano4::Spacetree::deriveSpawnLevel(DeepEnough, numberOfRanks);
    validateWithParams2(spawnLevel >= previousSpawnLevel, spawnLevel, numberOfRanks);
    previousSpawnLevel = spawnLevel;
  }

  const auto beyondOneLevel =
    peano4::Spacetree::deriveSpawnLevel(DeepEnough, static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2)) + 1);
  validate(beyondOneLevel > 2);
}

/**
 * A grid deeper than the lattice key range still returns a spawn level below its target level.
 */
void
peano4::tests::SpacetreeTest::testDeepGridsKeepTheirSpawnLevelInRange() {
  for (const int targetLevel : {20, 30, 60}) {
    for (const int numberOfRanks : {1, 2, 1000000}) {
      const auto spawnLevel = peano4::Spacetree::deriveSpawnLevel(targetLevel, numberOfRanks);
      validateWithParams2(spawnLevel >= 0, targetLevel, numberOfRanks);
      validateWithParams3(spawnLevel <= targetLevel - 1, spawnLevel, targetLevel, numberOfRanks);
    }
  }
}

/**
 * On the one-cell grid, a rank without cells reports the largest int as its minimum level.
 */
void
peano4::tests::SpacetreeTest::testARankWithoutCellsReportsNoLevel() {
  peano4::tests::buildRegularGrid(0);
  const auto& statistics = peano4::Spacetree::getInstance().getStatistics();
  const bool  ownsCells  = not peano4::Spacetree::getInstance().getCellConnectivity().getCells().isEmpty();

  validateEquals(peano4::tests::getGlobalSumOf(statistics.numberOfCells), 1);
  validateEquals(statistics.minimumLevel, ownsCells ? 0 : std::numeric_limits<int>::max());
  validateEquals(statistics.maximumLevel, 0);
}

/**
 * The statistics and the cell connectivity read as empty before the first regrid.
 */
void
peano4::tests::SpacetreeTest::testNoStatisticsStandBeforeTheFirstRegrid() {
  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.shutdown();
  spacetree.init(DomainOffset, DomainWidth);

  validateEquals(spacetree.getStatistics().numberOfCells, 0);
  validateEquals(spacetree.getStatistics().numberOfFaces, 0);
  validateEquals(spacetree.getStatistics().cellConnectivityBytes.getTotal(), std::size_t{0});
  validateEquals(spacetree.getCellConnectivity().getNumberOfSlots(), 0);
  validateEquals(spacetree.getCellConnectivity().getNumberOfFaces(), 0);
  validate(spacetree.getCellConnectivity().getCells().isEmpty());
  validate(spacetree.getCellConnectivity().getCells(peano4::Adjacency::Local).isEmpty());
  validate(spacetree.getCellConnectivity().getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Interior).isEmpty());
  validate(spacetree.getCellConnectivity().getLevelJumps(peano4::Adjacency::Local).isEmpty());
  validate(spacetree.getCellConnectivity().getNeighbours().isEmpty());
  validateEquals(spacetree.getCellAt(0, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
}

/**
 * The lookup returns the slot of every cell of the cell connectivity and -1 for every other level and
 * position.
 */
void
peano4::tests::SpacetreeTest::testTheCellLookupFindsEveryCellAndNothingElse() {
  constexpr int Depth = 2;
  peano4::tests::buildRegularGrid(Depth);
  const auto& spacetree        = peano4::Spacetree::getInstance();
  const auto& cellConnectivity = spacetree.getCellConnectivity();
  for (const int cell : cellConnectivity.getCells()) {
    validateEqualsWithParams1(spacetree.getCellAt(Depth, cellConnectivity.getCellLattice(cell)), cell, cell);
    validateEqualsWithParams1(spacetree.getCellAt(Depth - 1, cellConnectivity.getCellLattice(cell)), -1, cell);
  }
  validateEquals(spacetree.getCellAt(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
  validateEquals(spacetree.getCellAt(-1, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
}

void
peano4::tests::SpacetreeTest::testThePeriodicBoundariesReturnTheInitialisedValue() {
  std::bitset<DIMENSIONS> periodic;
  periodic.set();
  peano4::tests::buildRegularGrid(1, periodic);
  validate(peano4::Spacetree::getInstance().getPeriodicBoundaries().all());
  peano4::tests::buildRegularGrid(1);
  validate(peano4::Spacetree::getInstance().getPeriodicBoundaries().none());
}
