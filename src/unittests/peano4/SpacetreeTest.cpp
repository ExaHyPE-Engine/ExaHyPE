// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/Spacetree.h"

#include "peano4/CellConnectivity.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/topology/Tree.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);

  /**
   * Stores the spawn level that the tree derives for one target level and one rank count.
   */
  struct DerivedSpawnLevel final {
    int targetLevel{0};
    int numberOfRanks{0};
    int spawnLevel{0};
  };

  std::int64_t getNumberOfOwnershipUnitsOnLevel(const int level) {
    std::int64_t ownershipUnits = peano4::ChildrenPerCell;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      ownershipUnits *= peano4::topology::getCellsPerAxis(level);
    }
    return ownershipUnits;
  }

  /**
   * Returns the spawn level of every pair of a target level and a rank count.
   */
  std::vector<DerivedSpawnLevel> getDerivedSpawnLevels(
    const std::vector<int>& targetLevels,
    const std::vector<int>& rankCounts
  ) {
    std::vector<DerivedSpawnLevel> spawnLevels;
    for (const int targetLevel : targetLevels) {
      for (const int numberOfRanks : rankCounts) {
        spawnLevels.push_back(
          {targetLevel, numberOfRanks, peano4::topology::Tree::deriveSpawnLevel(targetLevel, numberOfRanks)}
        );
      }
    }
    return spawnLevels;
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

  void validateTheStatisticsOfTheLevelOneGrid(
    const peano4::Spacetree::Statistics& statistics,
    const peano4::CellConnectivity&      cellConnectivity
  ) {
    const bool ownsCells = not cellConnectivity.getCells().isEmpty();
    VALIDATE_EQUALS(statistics.numberOfFaces, cellConnectivity.getNumberOfFaces());
    VALIDATE_EQUALS(statistics.numberOfHangingFaces, 0);
    VALIDATE_EQUALS(statistics.maximumLevel, ownsCells ? 1 : 0);
    VALIDATE_EQUALS(statistics.minimumLevel, ownsCells ? 1 : std::numeric_limits<int>::max());
    VALIDATE_EQUALS(statistics.numberOfStationaryRegrids, 0);
    VALIDATE(statistics.cellConnectivityBytes.host > 0);
    VALIDATE(statistics.treeBytes > 0);
  }
} // namespace

UNIT_TEST("The spawn level deepens until its ownership units cover the ranks or the grid runs out of depth") {
  GIVEN("The target levels one to eight and rank counts one below, at and one above the ownership units of a level") {
    const int unitsOnRootLevel  = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(0));
    const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
    const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
    const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

    const std::vector<int> targetLevels{1, 2, 3, 4, 5, 6, 7, 8};
    const std::vector<int> rankCounts{
      1,
      2,
      unitsOnRootLevel - 1,
      unitsOnLevelOne,
      unitsOnLevelOne + 1,
      unitsOnLevelTwo,
      unitsOnLevelTwo + 1,
      unitsOnLevelThree
    };

    WHEN("The tree derives the spawn level of every target level for every rank count") {
      const auto spawnLevels = getDerivedSpawnLevels(targetLevels, rankCounts);

      THEN("Every spawn level offers an ownership unit per rank or lies right above the target level") {
        for (const auto& derived : spawnLevels) {
          const bool coversEveryRank    = getNumberOfOwnershipUnitsOnLevel(derived.spawnLevel) >= derived.numberOfRanks;
          const bool exhaustedGridDepth = derived.spawnLevel == std::max(derived.targetLevel - 1, 0);
          VALIDATE(coversEveryRank or exhaustedGridDepth, derived.targetLevel, derived.numberOfRanks);
        }
      }
    }
  }
}

UNIT_TEST("The spawn level stays strictly above the cells") {
  GIVEN("The target levels zero to ten and the rank counts 1, 4, 100 and 10000") {
    const std::vector<int> targetLevels{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::vector<int> rankCounts{1, 4, 100, 10000};

    WHEN("The tree derives the spawn level of every target level for every rank count") {
      const auto spawnLevels = getDerivedSpawnLevels(targetLevels, rankCounts);

      THEN("Every spawn level lies between the root and the level above the target level") {
        for (const auto& derived : spawnLevels) {
          VALIDATE(derived.spawnLevel >= 0, derived.targetLevel, derived.numberOfRanks);
          VALIDATE(
            derived.spawnLevel <= std::max(derived.targetLevel - 1, 0),
            derived.spawnLevel,
            derived.targetLevel,
            derived.numberOfRanks
          );
        }
      }
    }
  }
}

UNIT_TEST("A grid without a complete block level below the root uses the root as its spawn level") {
  GIVEN("The rank counts 1, 2 and 1000") {
    const std::vector<int> rankCounts{1, 2, 1000};

    WHEN("The tree derives the spawn levels of the target levels zero and one for every rank count") {
      std::vector<int> spawnLevelsOfTargetLevelZero;
      std::vector<int> spawnLevelsOfTargetLevelOne;
      for (const int numberOfRanks : rankCounts) {
        spawnLevelsOfTargetLevelZero.push_back(peano4::topology::Tree::deriveSpawnLevel(0, numberOfRanks));
        spawnLevelsOfTargetLevelOne.push_back(peano4::topology::Tree::deriveSpawnLevel(1, numberOfRanks));
      }

      THEN("Both target levels spawn on the root for every rank count") {
        for (std::size_t rankCount = 0; rankCount < rankCounts.size(); rankCount++) {
          VALIDATE_EQUALS(spawnLevelsOfTargetLevelZero[rankCount], 0);
          VALIDATE_EQUALS(spawnLevelsOfTargetLevelOne[rankCount], 0);
        }
      }
    }
  }
}

UNIT_TEST("The spawn level grows monotonically with the number of ranks") {
  GIVEN("The target level nine and rank counts on the ownership units of a level and one rank above them") {
    constexpr int DeepEnough = 9;

    const int unitsOnLevelOne   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(1));
    const int unitsOnLevelTwo   = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(2));
    const int unitsOnLevelThree = static_cast<int>(getNumberOfOwnershipUnitsOnLevel(3));

    const std::vector<int> rankCounts{
      2,
      unitsOnLevelOne,
      unitsOnLevelOne + 1,
      unitsOnLevelTwo,
      unitsOnLevelTwo + 1,
      unitsOnLevelThree,
      unitsOnLevelThree + 1
    };

    WHEN("The tree derives the spawn level of one rank, of every rank count and of one rank above level two") {
      const int        spawnLevelOfOneRank = peano4::topology::Tree::deriveSpawnLevel(DeepEnough, 1);
      std::vector<int> spawnLevelOfRankCount;
      for (const int numberOfRanks : rankCounts) {
        spawnLevelOfRankCount.push_back(peano4::topology::Tree::deriveSpawnLevel(DeepEnough, numberOfRanks));
      }
      const int beyondOneLevel = peano4::topology::Tree::deriveSpawnLevel(DeepEnough, unitsOnLevelTwo + 1);

      THEN("One rank spawns on the root and the spawn level rises with the rank count past level two") {
        VALIDATE_EQUALS(spawnLevelOfOneRank, 0);
        int previousSpawnLevel = spawnLevelOfOneRank;
        for (std::size_t rankCount = 0; rankCount < rankCounts.size(); rankCount++) {
          VALIDATE(
            spawnLevelOfRankCount[rankCount] >= previousSpawnLevel,
            spawnLevelOfRankCount[rankCount],
            rankCounts[rankCount]
          );
          previousSpawnLevel = spawnLevelOfRankCount[rankCount];
        }
        VALIDATE(beyondOneLevel > 2);
      }
    }
  }
}

UNIT_TEST("A grid deeper than the lattice key range still gets a spawn level below its target level") {
  GIVEN("The target levels 20, 30 and 60 and the rank counts 1, 2 and 1000000") {
    const std::vector<int> targetLevels{20, 30, 60};
    const std::vector<int> rankCounts{1, 2, 1000000};

    WHEN("The tree derives the spawn level of every target level for every rank count") {
      const auto spawnLevels = getDerivedSpawnLevels(targetLevels, rankCounts);

      THEN("Every spawn level lies between the root and the level above the target level") {
        for (const auto& derived : spawnLevels) {
          VALIDATE(derived.spawnLevel >= 0, derived.targetLevel, derived.numberOfRanks);
          VALIDATE(
            derived.spawnLevel <= derived.targetLevel - 1,
            derived.spawnLevel,
            derived.targetLevel,
            derived.numberOfRanks
          );
        }
      }
    }
  }
}

UNIT_TEST("The statistics and the cell connectivity read as empty before the first regrid") {
  GIVEN("The spacetree of the process") {
    auto& spacetree = peano4::Spacetree::getInstance();

    WHEN("The spacetree shuts down and initialises the unit domain without a regrid") {
      spacetree.shutdown();
      spacetree.init(DomainOffset, DomainWidth);

      THEN(
        "The statistics count zero and the cell connectivity returns zero slots, cells, faces, jumps and neighbours"
      ) {
        VALIDATE_EQUALS(spacetree.getStatistics().numberOfCells, 0);
        VALIDATE_EQUALS(spacetree.getStatistics().numberOfFaces, 0);
        VALIDATE_EQUALS(spacetree.getStatistics().cellConnectivityBytes.getTotal(), std::size_t{0});
        VALIDATE_EQUALS(spacetree.getCellConnectivity().getNumberOfSlots(), 0);
        VALIDATE_EQUALS(spacetree.getCellConnectivity().getNumberOfFaces(), 0);
        VALIDATE(spacetree.getCellConnectivity().getCells().isEmpty());
        VALIDATE(spacetree.getCellConnectivity().getCells(peano4::Adjacency::Local).isEmpty());
        VALIDATE(
          spacetree.getCellConnectivity().getFaces(peano4::Adjacency::Remote, peano4::FaceKind::Interior).isEmpty()
        );
        VALIDATE(spacetree.getCellConnectivity().getLevelJumps(peano4::Adjacency::Local).isEmpty());
        VALIDATE(spacetree.getCellConnectivity().getNeighbours().isEmpty());
        VALIDATE_EQUALS(spacetree.getCellAt(0, tarch::la::Vector<DIMENSIONS, int>(0)), -1);
      }
    }
  }
}

UNIT_TEST("On the one-cell grid a rank without cells reports the largest integer as its minimum level") {
  GIVEN("The depth zero with a regular grid of the root cell alone") {
    constexpr int Depth = 0;

    WHEN("The spacetree builds the regular grid of this depth and the ranks sum their cells") {
      peano4::unittests::buildRegularGrid(Depth);
      const auto& statistics = peano4::Spacetree::getInstance().getStatistics();
      const bool  ownsCells  = not peano4::Spacetree::getInstance().getCellConnectivity().getCells().isEmpty();
      const int   cells      = peano4::unittests::getGlobalSumOf(statistics.numberOfCells);

      THEN("One rank owns the cell of level zero and every other rank reports the largest integer as minimum") {
        VALIDATE_EQUALS(cells, 1);
        VALIDATE_EQUALS(statistics.minimumLevel, ownsCells ? 0 : std::numeric_limits<int>::max());
        VALIDATE_EQUALS(statistics.maximumLevel, 0);
      }
    }
  }
}

UNIT_TEST("A request set that the plan rejects keeps the plan and a repeated set answers from the cache") {
  GIVEN("A spacetree without a grid, the erase of the root and the refinement of a cell on level one") {
    auto& spacetree = peano4::Spacetree::getInstance();
    spacetree.shutdown();
    spacetree.init(DomainOffset, DomainWidth);

    peano4::RefinementRequest eraseRoot;
    eraseRoot.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    eraseRoot.level = 0;
    peano4::RefinementRequest refineCell;
    refineCell.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
    refineCell.level = 1;

    WHEN("The spacetree evaluates the empty set and the erase before the grid stands") {
      const bool emptySetChangesPlan = spacetree.requestsChangePlan({});
      const bool eraseChangesPlan    = spacetree.requestsChangePlan({eraseRoot});

      THEN("The non-empty set changes the plan and the empty set leaves it") {
        VALIDATE(not emptySetChangesPlan);
        VALIDATE(eraseChangesPlan);
      }
    }

    WHEN("The spacetree builds the regular grid of level one and the ranks sum their cells") {
      spacetree.scheduleRegularGridConstruction(1);
      peano4::Regrid regrid;
      const bool     built                     = spacetree.regrid({}, regrid);
      const auto&    cellConnectivity          = spacetree.getCellConnectivity();
      const auto&    statistics                = spacetree.getStatistics();
      const int      cells                     = peano4::unittests::getGlobalSumOf(statistics.numberOfCells);
      const int      cellsOfCellConnectivities = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
      const int      refinedCells              = peano4::unittests::getGlobalSumOf(statistics.numberOfRefinedCells);

      THEN("The regrid builds the children of the root and the statistics describe the grid of level one") {
        VALIDATE(built);
        VALIDATE_EQUALS(cells, peano4::ChildrenPerCell);
        VALIDATE_EQUALS(cellsOfCellConnectivities, peano4::ChildrenPerCell);
        VALIDATE_EQUALS(refinedCells, 1);
        validateTheStatisticsOfTheLevelOneGrid(statistics, cellConnectivity);
      }

      AND_WHEN("The spacetree evaluates request sets on the standing grid") {
        const bool emptySetChangesPlan                = spacetree.requestsChangePlan({});
        const bool eraseChangesPlan                   = spacetree.requestsChangePlan({eraseRoot});
        const bool repeatedEraseChangesPlan           = spacetree.requestsChangePlan({eraseRoot});
        const bool refinementChangesPlan              = spacetree.requestsChangePlan({refineCell});
        const bool eraseAndRefinementChangePlan       = spacetree.requestsChangePlan({eraseRoot, refineCell});
        const bool eraseAfterTheRefinementChangesPlan = spacetree.requestsChangePlan({eraseRoot});

        THEN("The empty set and the erase of the root keep the plan and every refining set changes it") {
          VALIDATE(not emptySetChangesPlan);
          VALIDATE(not eraseChangesPlan);
          VALIDATE(not repeatedEraseChangesPlan);
          VALIDATE(refinementChangesPlan);
          VALIDATE(eraseAndRefinementChangePlan);
          VALIDATE(not eraseAfterTheRefinementChangesPlan);
        }

        AND_WHEN("The spacetree regrids with the erase of the root and then with the empty set") {
          const bool regriddedWithErase                = spacetree.regrid({eraseRoot}, regrid);
          const int  stationaryRegridsAfterTheErase    = statistics.numberOfStationaryRegrids;
          const bool regriddedWithEmptySet             = spacetree.regrid({}, regrid);
          const int  stationaryRegridsAfterTheEmptySet = statistics.numberOfStationaryRegrids;

          THEN("Both regrids keep the grid and each counts as one more stationary regrid") {
            VALIDATE(not regriddedWithErase);
            VALIDATE_EQUALS(stationaryRegridsAfterTheErase, 1);
            VALIDATE(not regriddedWithEmptySet);
            VALIDATE_EQUALS(stationaryRegridsAfterTheEmptySet, 2);
          }
        }
      }
    }
  }
}

UNIT_TEST("With live data a regrid fills every new cell by one prolongation, one restriction or one receive") {
  GIVEN("The regular grid of depth one with live data and its number of cells over the ranks") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    auto&       spacetree        = peano4::Spacetree::getInstance();
    const auto& cellConnectivity = spacetree.getCellConnectivity();
    const auto& statistics       = spacetree.getStatistics();
    const int   cellsBefore      = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
    spacetree.markPersistentDataLive();

    WHEN("The cell in the centre refines") {
      peano4::Regrid regrid;
      const bool     refined = spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, regrid);

      const int fineCells =
        peano4::unittests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth + 1));
      const int prolongedOrReceivedCells = peano4::unittests::getGlobalSumOf(
        static_cast<int>(regrid.prolongations.size()) + getNumberOfReceivedSlots(regrid)
      );
      const int coarseCellsAfterRefinement =
        peano4::unittests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth));
      const int freedSlots           = peano4::unittests::getGlobalSumOf(static_cast<int>(regrid.freedSlots.size()));
      const int cellsAfterRefinement = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);

      const auto restrictionsOfTheRefinement = regrid.restrictions.size();
      const int  maximumLevelAfterRefinement = statistics.maximumLevel;
      const bool ownsFineCells               = getNumberOfLocalCellsOfLevel(cellConnectivity, Depth + 1) > 0;
      spacetree.finishRegrid();

      THEN("The regrid prolongs or receives every fine cell and frees the slots of the refined coarse cells") {
        VALIDATE_EQUALS(cellsBefore, peano4::ChildrenPerCell);
        VALIDATE(refined);
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(prolongedOrReceivedCells, fineCells);
        VALIDATE_EQUALS(freedSlots, cellsBefore - coarseCellsAfterRefinement);
        VALIDATE_EQUALS(cellsAfterRefinement, coarseCellsAfterRefinement + fineCells);
        VALIDATE_EQUALS(restrictionsOfTheRefinement, std::size_t{0});
        VALIDATE_EQUALS(maximumLevelAfterRefinement == Depth + 1, ownsFineCells, maximumLevelAfterRefinement);
      }

      AND_WHEN("Every cell erases back") {
        const bool erased = spacetree.regrid(peano4::unittests::getErasureOfEveryCell(Depth), regrid);

        const int coarseCellsAfterErase =
          peano4::unittests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth));
        const int restrictedOrReceivedCells = peano4::unittests::getGlobalSumOf(
          static_cast<int>(regrid.restrictions.size()) + getNumberOfReceivedSlots(regrid)
        );
        const int cellsAfterErase = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
        const int fineCellsAfterErase =
          peano4::unittests::getGlobalSumOf(getNumberOfLocalCellsOfLevel(cellConnectivity, Depth + 1));

        const auto prolongationsOfTheErase = regrid.prolongations.size();
        spacetree.finishRegrid();

        THEN("The regrid restricts or receives every new coarse cell and the erase prolongs zero cells") {
          VALIDATE(erased);
          VALIDATE(coarseCellsAfterErase > coarseCellsAfterRefinement);
          VALIDATE_EQUALS(restrictedOrReceivedCells, coarseCellsAfterErase - coarseCellsAfterRefinement);
          VALIDATE_EQUALS(cellsAfterErase, coarseCellsAfterErase + fineCellsAfterErase);
          VALIDATE_EQUALS(prolongationsOfTheErase, std::size_t{0});
        }
      }
    }
  }
}

UNIT_TEST("After a refinement and the erase back the tree bytes count the standing tree and the spare tree") {
  GIVEN("The regular grid of depth one with live data and the tree bytes of this one tree") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    auto&             spacetree      = peano4::Spacetree::getInstance();
    const std::size_t bytesOfOneTree = spacetree.getStatistics().treeBytes;
    spacetree.markPersistentDataLive();

    WHEN("The spacetree refines around the centre cell and erases every cell afterwards") {
      peano4::Regrid regrid;
      const bool     refinementChangesTheGrid =
        spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, regrid);
      spacetree.finishRegrid();
      const bool eraseChangesTheGrid = spacetree.regrid(peano4::unittests::getErasureOfEveryCell(Depth), regrid);
      spacetree.finishRegrid();
      const std::size_t treeBytes = spacetree.getStatistics().treeBytes;

      THEN("Both regrids change the grid and the tree bytes cover more than one and a half trees") {
        VALIDATE(bytesOfOneTree > sizeof(peano4::topology::Tree), bytesOfOneTree);
        VALIDATE(refinementChangesTheGrid);
        VALIDATE(eraseChangesTheGrid);
        VALIDATE(treeBytes > 3 * bytesOfOneTree / 2, treeBytes, bytesOfOneTree);
      }
    }
  }
}

UNIT_TEST("finishRegrid() adds the growth of the free list after an erase to the cell connectivity bytes") {
  GIVEN("The regular grid of depth one with live data, refined around its centre cell") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    auto&       spacetree  = peano4::Spacetree::getInstance();
    const auto& statistics = spacetree.getStatistics();
    spacetree.markPersistentDataLive();
    peano4::Regrid regrid;
    const bool     refinementChangesTheGrid =
      spacetree.regrid({peano4::unittests::getRefinementOfCentreCell(Depth)}, regrid);
    spacetree.finishRegrid();

    WHEN(
      "The spacetree erases every cell and the ranks sum the growth of the cell connectivity bytes in finishRegrid()"
    ) {
      const bool        eraseChangesTheGrid = spacetree.regrid(peano4::unittests::getErasureOfEveryCell(Depth), regrid);
      const std::size_t bytesBeforeFinish   = statistics.cellConnectivityBytes.host;
      spacetree.finishRegrid();
      const std::size_t bytesAfterFinish = statistics.cellConnectivityBytes.host;
      const int         grownBytes       = static_cast<int>(bytesAfterFinish - bytesBeforeFinish);
      const int         globalGrowth     = peano4::unittests::getGlobalSumOf(grownBytes);

      THEN("Calls of finishRegrid() never shrink the cell connectivity bytes and their summed growth is positive") {
        VALIDATE(refinementChangesTheGrid);
        VALIDATE(eraseChangesTheGrid);
        VALIDATE(bytesAfterFinish >= bytesBeforeFinish, bytesAfterFinish, bytesBeforeFinish);
        VALIDATE(globalGrowth > 0, grownBytes, globalGrowth);
      }
    }
  }
}

UNIT_TEST("The tree bytes count the plan that requestsChangePlan() evaluates and keeps for the next regrid") {
  GIVEN("The regular grid of depth one and its tree bytes") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    auto&             spacetree   = peano4::Spacetree::getInstance();
    const std::size_t bytesBefore = spacetree.getStatistics().treeBytes;

    WHEN("The spacetree evaluates the refinement around the centre cell") {
      const bool changesThePlan = spacetree.requestsChangePlan({peano4::unittests::getRefinementOfCentreCell(Depth)});
      const std::size_t bytesAfter = spacetree.getStatistics().treeBytes;

      THEN("The refinement changes the plan and the tree bytes grow") {
        VALIDATE(changesThePlan);
        VALIDATE(bytesAfter > bytesBefore, bytesAfter, bytesBefore);
      }
    }
  }
}

UNIT_TEST(
  "The cell lookup returns the slot of every cell of the cell connectivity and -1 for every other level and position"
) {
  GIVEN("The regular grid of depth two") {
    constexpr int Depth = 2;
    peano4::unittests::buildRegularGrid(Depth);
    const auto& spacetree        = peano4::Spacetree::getInstance();
    const auto& cellConnectivity = spacetree.getCellConnectivity();

    WHEN("The test looks up every cell on its level and one level up and the origin below the grid and on level -1") {
      std::vector<int> cells;
      std::vector<int> slotsOnTheLevelOfTheCell;
      std::vector<int> slotsOneLevelUp;
      for (const int cell : cellConnectivity.getCells()) {
        cells.push_back(cell);
        slotsOnTheLevelOfTheCell.push_back(spacetree.getCellAt(Depth, cellConnectivity.getCellLattice(cell)));
        slotsOneLevelUp.push_back(spacetree.getCellAt(Depth - 1, cellConnectivity.getCellLattice(cell)));
      }
      const int slotBelowTheGrid    = spacetree.getCellAt(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(0));
      const int slotOnLevelMinusOne = spacetree.getCellAt(-1, tarch::la::Vector<DIMENSIONS, int>(0));

      THEN("Every cell finds its own slot on its level and every other lookup returns -1") {
        for (std::size_t entry = 0; entry < cells.size(); entry++) {
          VALIDATE_EQUALS(slotsOnTheLevelOfTheCell[entry], cells[entry], cells[entry]);
          VALIDATE_EQUALS(slotsOneLevelUp[entry], -1, cells[entry]);
        }
        VALIDATE_EQUALS(slotBelowTheGrid, -1);
        VALIDATE_EQUALS(slotOnLevelMinusOne, -1);
      }
    }
  }
}

UNIT_TEST("The spacetree returns the periodic boundaries of its initialisation") {
  GIVEN("Periodic boundaries along every axis") {
    std::bitset<DIMENSIONS> periodic;
    periodic.set();

    WHEN("The spacetree builds a grid of depth one with these boundaries") {
      peano4::unittests::buildRegularGrid(1, periodic);
      const bool everyAxisIsPeriodic = peano4::Spacetree::getInstance().getPeriodicBoundaries().all();

      THEN("Every axis is periodic") { VALIDATE(everyAxisIsPeriodic); }

      AND_WHEN("The spacetree builds a grid of depth one without periodic boundaries") {
        peano4::unittests::buildRegularGrid(1);
        const bool noAxisIsPeriodic = peano4::Spacetree::getInstance().getPeriodicBoundaries().none();

        THEN("The set of periodic axes is empty") { VALIDATE(noAxisIsPeriodic); }
      }
    }
  }
}
