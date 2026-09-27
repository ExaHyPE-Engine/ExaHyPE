// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeTest.h"

#include "peano4/Partition.h"
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

  int getNumberOfLocalCellsOfLevel(const peano4::Partition& partition, const int level) {
    return partition.getCells(peano4::Dependence::Local, level).size
      + partition.getCells(peano4::Dependence::Remote, level).size;
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
  testMethod(testRejectedRequestsChangeNoPlan);
  testMethod(testARegridWithLiveDataReturnsTheSlotMoves);
  testMethod(testTheCellLookupFindsEveryCellAndNothingElse);
  testMethod(testThePeriodicBoundariesReturnTheInitialisedValue);
}

/**
 * Every non-empty request set changes the plan before the tree stands, and on a standing level-one
 * grid an erase of the root changes nothing, a refine changes the plan and a repeated set answers
 * from the cache.
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
  const auto& partition  = spacetree.getPartition();
  const auto& statistics = spacetree.getStatistics();
  validateEquals(peano4::tests::getGlobalSumOf(statistics.numberOfCells), peano4::ChildrenPerCell);
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), peano4::ChildrenPerCell);
  validateEquals(peano4::tests::getGlobalSumOf(statistics.numberOfRefinedCells), 1);
  validateEquals(statistics.numberOfFaces, partition.getNumberOfFaces());
  validateEquals(statistics.numberOfHangingFaces, 0);
  validateEquals(statistics.maximumLevel, 1);
  validateEquals(statistics.minimumLevel, 1);
  validateEquals(statistics.numberOfStationaryRegrids, 0);
  validate(statistics.partitionBytes.host > 0);
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
 * With live data a refinement keeps the slots of the coarse cells and fills every new fine cell by
 * one prolongation or one receive, and an erase fills every new coarse cell by one restriction or
 * one receive.
 */
void
peano4::tests::SpacetreeTest::testARegridWithLiveDataReturnsTheSlotMoves() {
  constexpr int Depth = 1;
  peano4::tests::buildRegularGrid(Depth);
  auto&       spacetree   = peano4::Spacetree::getInstance();
  const auto& partition   = spacetree.getPartition();
  const auto& statistics  = spacetree.getStatistics();
  const int   cellsBefore = peano4::tests::getGlobalSumOf(partition.getCells().size);
  validateEquals(cellsBefore, peano4::ChildrenPerCell);

  spacetree.markPersistentDataLive();

  peano4::Regrid regrid;
  validateTrue(spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid));
  const int fineCells = peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(partition, Depth + 1));
  validate(fineCells > 0);
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.prolongations.size()) + getNumberOfReceivedSlots(regrid)),
    fineCells
  );
  validate(regrid.restrictions.empty());
  const int coarseCellsAfterRefinement = peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(partition, Depth));
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.freedSlots.size())),
    cellsBefore - coarseCellsAfterRefinement
  );
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), coarseCellsAfterRefinement + fineCells);
  validateEquals(statistics.maximumLevel, Depth + 1);
  spacetree.finishRegrid();

  validateTrue(spacetree.regrid(peano4::tests::getErasureOfEveryCell(Depth), regrid));
  const int coarseCellsAfterErase = peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(partition, Depth));
  validate(coarseCellsAfterErase > coarseCellsAfterRefinement);
  validate(regrid.prolongations.empty());
  validateEquals(
    peano4::tests::getGlobalSumOf(static_cast<int>(regrid.restrictions.size()) + getNumberOfReceivedSlots(regrid)),
    coarseCellsAfterErase - coarseCellsAfterRefinement
  );
  validateEquals(
    peano4::tests::getGlobalSumOf(partition.getCells().size),
    coarseCellsAfterErase + peano4::tests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(partition, Depth + 1))
  );
  spacetree.finishRegrid();
}

/** The spawn level deepens until its ownership units cover the ranks or the grid runs out of depth. */
void
peano4::tests::SpacetreeTest::testTheSpawnLevelOffersEnoughOwnershipUnits() {
  const int unitsOnRootLevel  = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(0));
  const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
  const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
  const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

  for (int targetLevel = 1; targetLevel <= 8; targetLevel++) {
    // The rank counts sit right below and right above the ownership units of a level.
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

/** The spawn level stays strictly above the cells. */
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

/** A grid with no complete block level below the root has to use the root as its spawn level. */
void
peano4::tests::SpacetreeTest::testShallowGridsUseTheRootSpawnLevel() {
  for (const int numberOfRanks : {1, 2, 1000}) {
    validateEquals(peano4::Spacetree::deriveSpawnLevel(0, numberOfRanks), 0);
    validateEquals(peano4::Spacetree::deriveSpawnLevel(1, numberOfRanks), 0);
  }
}

/** The spawn level increases monotonically with the number of ranks. */
void
peano4::tests::SpacetreeTest::testTheSpawnLevelGrowsWithTheRankCount() {
  constexpr int DeepEnough = 9;

  const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
  const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
  const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

  int previousSpawnLevel = peano4::Spacetree::deriveSpawnLevel(DeepEnough, 1);
  validateEquals(previousSpawnLevel, 0);

  // The rank counts sit on the ownership units of a level and one rank above them.
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

/** A grid deeper than the lattice key range still returns a spawn level below its target level. */
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

/** The statistics and the partition read as empty before the first regrid. */
void
peano4::tests::SpacetreeTest::testNoStatisticsStandBeforeTheFirstRegrid() {
  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.shutdown();
  spacetree.init(DomainOffset, DomainWidth);

  validateEquals(spacetree.getStatistics().numberOfCells, 0);
  validateEquals(spacetree.getStatistics().numberOfFaces, 0);
  validateEquals(spacetree.getStatistics().partitionBytes.getTotal(), std::size_t{0});
  validateEquals(spacetree.getPartition().getNumberOfSlots(), 0);
  validateEquals(spacetree.getPartition().getNumberOfFaces(), 0);
  validate(spacetree.getPartition().getCells().isEmpty());
  validate(spacetree.getPartition().getCells(peano4::Dependence::Local).isEmpty());
  validate(spacetree.getPartition().getFaces(peano4::Dependence::Remote, peano4::FaceKind::Interior).isEmpty());
  validate(spacetree.getPartition().getLevelJumps(peano4::Dependence::Local).isEmpty());
  validate(spacetree.getPartition().getNeighbours().isEmpty());
  validateEquals(spacetree.getCellAt(0, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
}

/** The lookup returns the slot of every cell of the partition and -1 for every other level and position. */
void
peano4::tests::SpacetreeTest::testTheCellLookupFindsEveryCellAndNothingElse() {
  constexpr int Depth = 2;
  peano4::tests::buildRegularGrid(Depth);
  const auto& spacetree = peano4::Spacetree::getInstance();
  const auto& partition = spacetree.getPartition();
  for (const int cell : partition.getCells()) {
    validateEqualsWithParams1(spacetree.getCellAt(Depth, partition.getCellLattice(cell)), cell, cell);
    validateEqualsWithParams1(spacetree.getCellAt(Depth - 1, partition.getCellLattice(cell)), -1, cell);
  }
  validateEquals(spacetree.getCellAt(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
  validateEquals(spacetree.getCellAt(-1, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
}

/** The spacetree returns the periodic boundaries of its init(). */
void
peano4::tests::SpacetreeTest::testThePeriodicBoundariesReturnTheInitialisedValue() {
  std::bitset<DIMENSIONS> periodic;
  periodic.set();
  peano4::tests::buildRegularGrid(1, periodic);
  validate(peano4::Spacetree::getInstance().getPeriodicBoundaries().all());
  peano4::tests::buildRegularGrid(1);
  validate(peano4::Spacetree::getInstance().getPeriodicBoundaries().none());
}
