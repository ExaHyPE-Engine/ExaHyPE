// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TopologyManagerTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/topology/TopologyManager.h"
#include "peano4/topology/Tree.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::int64_t ownershipUnitsOnLevel(const int level) {
    std::int64_t ownershipUnits = THREE_POWER_D;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      ownershipUnits *= peano4::topology::cellsPerAxis(level);
    }
    return ownershipUnits;
  }

  /** Observes a sweep without data and raises no request. */
  class EmptyObserver final : public peano4::grid::GridTraversalObserver {
  public:
    peano4::grid::GridTraversalObserver* clone(const int treeId) override {
      static_cast<void>(treeId);
      return new EmptyObserver();
    }
    std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return {}; }
    void                                        beginTraversal(
                                             const tarch::la::Vector<DIMENSIONS, double>& x,
                                             const tarch::la::Vector<DIMENSIONS, double>& h
                                           ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void endTraversal(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void enterNode(const peano4::grid::GridTraversalEvent& event) override { static_cast<void>(event); }
    void leaveNode(const peano4::grid::GridTraversalEvent& event) override { static_cast<void>(event); }
  };
} // namespace

peano4::topology::tests::TopologyManagerTest::TopologyManagerTest() :
  TestCase("peano4::topology::tests::TopologyManagerTest") {}

void peano4::topology::tests::TopologyManagerTest::run() {
  testMethod(testTheSpawnLevelOffersEnoughOwnershipUnits);
  testMethod(testTheSpawnLevelStaysAboveTheLeaves);
  testMethod(testShallowGridsUseTheRootSpawnLevel);
  testMethod(testTheSpawnLevelGrowsWithTheRankCount);
  testMethod(testDeepGridsKeepTheirSpawnLevelInRange);
  testMethod(testNoTaskLoadStandsBeforeTheFirstSweep);
  testMethod(testTheRegridRealisationCoversEveryDelta);
  testMethod(testRejectedControlEventsChangeNoPlan);
  testMethod(testTheDomainAccessorsReturnTheInitialisedDomain);
}

/**
 * Before the tree stands every non-empty set counts as a change. On a
 * standing level-one grid an erase of the root changes nothing, a refine of a
 * leaf changes the plan, and the rejected set answers from the cache when it
 * repeats.
 */
void peano4::topology::tests::TopologyManagerTest::testRejectedControlEventsChangeNoPlan() {
  auto& manager = peano4::topology::TopologyManager::getInstance();

  peano4::grid::GridControlEvent eraseRoot;
  eraseRoot.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
  eraseRoot.level = 0;
  peano4::grid::GridControlEvent refineLeaf;
  refineLeaf.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
  refineLeaf.level = 1;

  validateFalse(manager.controlEventsChangePlan({}));
  validateTrue(manager.controlEventsChangePlan({eraseRoot}));

  manager.scheduleRegularGridConstruction(1);
  EmptyObserver observer;
  manager.traverse(observer);
  // The ranks partition the level-one grid, so the sum over the ranks counts
  // every cell once.
  const int localLeaves  = manager.getGridStatistics().numberOfLocalUnrefinedCells;
  int       globalLeaves = 0;
  tarch::mpi::allreduce(
    &localLeaves,
    &globalLeaves,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  validateEquals(globalLeaves, THREE_POWER_D);

  validateFalse(manager.controlEventsChangePlan({}));
  validateFalse(manager.controlEventsChangePlan({eraseRoot}));
  validateFalse(manager.controlEventsChangePlan({eraseRoot}));
  validateTrue(manager.controlEventsChangePlan({refineLeaf}));
  validateTrue(manager.controlEventsChangePlan({eraseRoot, refineLeaf}));
  validateFalse(manager.controlEventsChangePlan({eraseRoot}));
}

/**
 * Every delta and partition state selects the regrid realisation the decision
 * table names.
 */
void peano4::topology::tests::TopologyManagerTest::testTheRegridRealisationCoversEveryDelta() {
  using Manager     = peano4::topology::TopologyManager;
  using Realisation = Manager::RegridRealisation;

  for (const bool partitionMoved : {false, true}) {
    for (const bool gapsStayBounded : {false, true}) {
      // An erase prunes where the cells stand, so it needs a still partition
      // and room among the gaps its earlier prunes left.
      const auto erase = Manager::selectRegridRealisation(true, false, partitionMoved, gapsStayBounded);
      validate(
        erase == (not partitionMoved and gapsStayBounded ? Realisation::EraseInPlace : Realisation::EraseOnRebuiltTree)
      );

      // A refinement only appends, so the gaps never block it.
      const auto refine = Manager::selectRegridRealisation(false, true, partitionMoved, gapsStayBounded);
      validate(refine == (partitionMoved ? Realisation::MixedWithAppendedUnion : Realisation::RefineInPlace));

      // A mixed delta both appends and prunes, so it carries the erase's
      // conditions; a moved partition hops to the new structure.
      const auto mixed = Manager::selectRegridRealisation(false, false, partitionMoved, gapsStayBounded);
      if (partitionMoved) {
        validate(mixed == Realisation::MixedWithAppendedUnion);
      } else {
        validate(mixed == (gapsStayBounded ? Realisation::MixedInPlace : Realisation::MixedWithAppendedUnion));
      }
    }
  }
}

/**
 * The spawn level deepens until its ownership units cover the ranks or the grid
 * runs out of depth.
 */
void peano4::topology::tests::TopologyManagerTest::testTheSpawnLevelOffersEnoughOwnershipUnits() {
  for (int targetLevel = 1; targetLevel <= 8; targetLevel++) {
    for (const int numberOfRanks : {1, 2, 8, 81, 82, 729, 730, 6561}) {
      const auto spawnLevel         = peano4::topology::TopologyManager::deriveSpawnLevel(targetLevel, numberOfRanks);
      const bool coversEveryRank    = ownershipUnitsOnLevel(spawnLevel) >= numberOfRanks;
      const bool exhaustedGridDepth = spawnLevel == std::max(targetLevel - 1, 0);
      validateWithParams2(coversEveryRank or exhaustedGridDepth, targetLevel, numberOfRanks);
    }
  }
}

/** The spawn level stays strictly above the leaves. */
void peano4::topology::tests::TopologyManagerTest::testTheSpawnLevelStaysAboveTheLeaves() {
  for (int targetLevel = 0; targetLevel <= 10; targetLevel++) {
    for (const int numberOfRanks : {1, 4, 100, 10000}) {
      const auto spawnLevel = peano4::topology::TopologyManager::deriveSpawnLevel(targetLevel, numberOfRanks);
      validateWithParams2(spawnLevel >= 0, targetLevel, numberOfRanks);
      validateWithParams3(spawnLevel <= std::max(targetLevel - 1, 0), spawnLevel, targetLevel, numberOfRanks);
    }
  }
}

/**
 * A grid with no complete block level below the root has to use the root as
 * its spawn level.
 */
void peano4::topology::tests::TopologyManagerTest::testShallowGridsUseTheRootSpawnLevel() {
  for (const int numberOfRanks : {1, 2, 1000}) {
    validateEquals(peano4::topology::TopologyManager::deriveSpawnLevel(0, numberOfRanks), 0);
    validateEquals(peano4::topology::TopologyManager::deriveSpawnLevel(1, numberOfRanks), 0);
  }
}

/** The spawn level increases monotonically with the number of ranks. */
void peano4::topology::tests::TopologyManagerTest::testTheSpawnLevelGrowsWithTheRankCount() {
  constexpr int DeepEnough = 9;

  int previousSpawnLevel = peano4::topology::TopologyManager::deriveSpawnLevel(DeepEnough, 1);
  validateEquals(previousSpawnLevel, 0);

  for (const int numberOfRanks : {2, 81, 82, 729, 730, 6561, 6562}) {
    const auto spawnLevel = peano4::topology::TopologyManager::deriveSpawnLevel(DeepEnough, numberOfRanks);
    validateWithParams2(spawnLevel >= previousSpawnLevel, spawnLevel, numberOfRanks);
    previousSpawnLevel = spawnLevel;
  }

  const auto beyondOneLevel =
    peano4::topology::TopologyManager::deriveSpawnLevel(DeepEnough, static_cast<int>(ownershipUnitsOnLevel(2)) + 1);
  validate(beyondOneLevel > 2);
}

/**
 * A grid deeper than the lattice key range still returns a spawn level below
 * its target level.
 */
void peano4::topology::tests::TopologyManagerTest::testDeepGridsKeepTheirSpawnLevelInRange() {
  for (const int targetLevel : {20, 30, 60}) {
    for (const int numberOfRanks : {1, 2, 1000000}) {
      const auto spawnLevel = peano4::topology::TopologyManager::deriveSpawnLevel(targetLevel, numberOfRanks);
      validateWithParams2(spawnLevel >= 0, targetLevel, numberOfRanks);
      validateWithParams3(spawnLevel <= targetLevel - 1, spawnLevel, targetLevel, numberOfRanks);
    }
  }
}

/**
 * The subtree task load and the grid statistics read as zero before the first
 * sweep.
 */
void peano4::topology::tests::TopologyManagerTest::testNoTaskLoadStandsBeforeTheFirstSweep() {
  const auto& manager = peano4::topology::TopologyManager::getInstance();

  const auto load = manager.getSubtreeTaskLoad();
  validateEquals(load.numberOfTasks, 0);
  validateEquals(load.minimumCells, 0);
  validateEquals(load.maximumCells, 0);
  validateEquals(load.totalCells, 0);
  validateNumericalEquals(load.taskSeconds, 0.0);
  validateNumericalEquals(load.windowSeconds, 0.0);

  validateNumericalEquals(manager.getAccumulatedTaskSeconds(), 0.0);
  validateEquals(manager.getGridStatistics().numberOfLocalUnrefinedCells, 0);
  validateEquals(manager.getGridStatistics().numberOfVertices, 0);
}

/** The manager returns the unit domain the test driver initialises it with. */
void peano4::topology::tests::TopologyManagerTest::testTheDomainAccessorsReturnTheInitialisedDomain() {
  const auto& manager = peano4::topology::TopologyManager::getInstance();
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(manager.getDomainOffset()(axis), 0.0);
    validateNumericalEquals(manager.getDomainWidth()(axis), 1.0);
  }
  validate(manager.getPeriodicBC().none());
}
