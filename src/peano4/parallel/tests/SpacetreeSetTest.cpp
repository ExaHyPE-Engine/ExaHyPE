// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSetTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/mpi/Rank.h"

#include <algorithm>
#include <set>

namespace {
  constexpr double DomainOffset = 10.0;
  constexpr double DomainSize   = 1.0;

  /**
   * Refining the whole box to this resolution yields a uniform level-2 grid:
   * the level-1 cells (h = 1/3) are coarser than the target, the level-2
   * cells (h = 1/9) are not.
   */
  constexpr double UniformTargetH = 0.15;

  /**
   * Refining the corner box to this resolution adds one more level there: the
   * level-2 cells are coarser than the target, the level-3 cells (h = 1/27)
   * are not.
   */
  constexpr double CornerTargetH = 0.05;
  constexpr double CornerExtent  = 0.12;

  class ScenarioObserver final : public peano4::grid::GridTraversalObserver {
  public:
    std::vector<peano4::grid::GridControlEvent> events;

    void enterNode(const peano4::grid::GridTraversalEvent&) override {}
    void leaveNode(const peano4::grid::GridTraversalEvent&) override {}

    peano4::grid::GridTraversalObserver* clone(int) override { return new ScenarioObserver(*this); }

    std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return events; }

    void beginTraversal(
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&
    ) override {}
    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  peano4::grid::GridControlEvent createRefineEvent(double offset, double extent, double targetH) {
    peano4::grid::GridControlEvent result;
    result.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; d++) {
      result.offset[d]  = offset;
      result.width[d]   = extent;
      result.targetH[d] = targetH;
    }
    return result;
  }

  peano4::grid::GridControlEvent createEraseEvent(double offset, double extent, double targetH) {
    peano4::grid::GridControlEvent result;
    result.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    for (int d = 0; d < DIMENSIONS; d++) {
      result.offset[d]  = offset;
      result.width[d]   = extent;
      result.targetH[d] = targetH;
    }
    return result;
  }

  void traverseSweeps(peano4::parallel::SpacetreeSet& set, peano4::grid::GridTraversalObserver& observer, int sweeps) {
    for (int sweep = 0; sweep < sweeps; sweep++) {
      set.traverse(observer);
    }
  }
} // namespace

peano4::parallel::tests::SpacetreeSetTest::SpacetreeSetTest() :
  TestCase("peano4::parallel::tests::SpacetreeSetTest") {}

void peano4::parallel::tests::SpacetreeSetTest::run() {
  testMethod(testRegularGridLoadBalancingMatchesNonPeriodicRun);
  testMethod(testCornerRefinementMirrorsAcrossPeriodicBoundaries);
  testMethod(testMultiTreePeriodicGridMatchesSingleTree);
  testMethod(testEraseAtPeriodicBoundaryRestoresRegularGrid);
}

/**
 * Run one complete scenario: create a root tree over the box
 * [10,11]^DIMENSIONS, build a uniform level-2 grid, statically refine a
 * corner region if requested, split into targetTrees trees, optionally
 * erase the refined region again, and settle. Returns the family's
 * aggregated statistics and removes every trace of the scenario from the
 * spacetree set afterwards.
 *
 * All ranks execute the same number of collective traversals; tree
 * creation, splits, and the returned statistics are confined to the global
 * master, so the test runs identically in serial and MPI configurations.
 */
peano4::parallel::tests::SpacetreeSetTest::ScenarioResult peano4::parallel::tests::SpacetreeSetTest::runScenario(
  bool periodic,
  int  targetTrees,
  bool refineCorner,
  bool eraseCornerAgain
) {
  auto&      set            = SpacetreeSet::getInstance();
  auto&      node           = Node::getInstance();
  const int  rank           = tarch::mpi::Rank::getInstance().getRank();
  const bool isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();

  std::bitset<DIMENSIONS> periodicAxes;
  if (periodic) {
    periodicAxes.set();
  }

  // The set-level periodic configuration drives the collective routing
  // rebuild, so every rank flips it for the scenario and restores it below.
  const std::bitset<DIMENSIONS> backupPeriodicBC = set._periodicBC;
  set._periodicBC                                = periodicAxes;

  int rootId = -1;
  if (isGlobalMaster) {
    rootId = node.reserveId(rank, 0);
    set._spacetrees.emplace_back(
      rootId,
      tarch::la::Vector<DIMENSIONS, double>(DomainOffset),
      tarch::la::Vector<DIMENSIONS, double>(DomainSize),
      periodicAxes
    );
    set.registerLocalSpacetree(set._spacetrees.back());
  }

  ScenarioResult result;

  ScenarioObserver observer;
  observer.events = {createRefineEvent(DomainOffset, DomainSize, UniformTargetH)};
  if (refineCorner) {
    observer.events.push_back(createRefineEvent(DomainOffset, CornerExtent, CornerTargetH));
  }
  traverseSweeps(set, observer, 8);

  if (targetTrees > 1 and isGlobalMaster) {
    const int cells = set.getGridStatistics(rootId).numberOfLocalUnrefinedCells;
    for (int split = 1; split < targetTrees; split++) {
      result.splitsSucceeded &= set.split(
        rootId,
        peano4::grid::SplitInstruction{cells / targetTrees, peano4::grid::SplitInstruction::Mode::BottomUp},
        rank
      );
    }
  }
  traverseSweeps(set, observer, 6);

  if (eraseCornerAgain) {
    observer.events = {
      createRefineEvent(DomainOffset, DomainSize, UniformTargetH),
      createEraseEvent(DomainOffset, DomainSize, UniformTargetH)
    };
    traverseSweeps(set, observer, 8);
  }

  std::set<int> family;
  if (isGlobalMaster) {
    family.insert(rootId);
    bool familyGrew = true;
    while (familyGrew) {
      familyGrew = false;
      for (const auto& tree : set._spacetrees) {
        if (family.count(tree.getId()) == 0 and family.count(tree.getMasterId()) > 0) {
          family.insert(tree.getId());
          familyGrew = true;
        }
      }
    }

    for (const int treeId : family) {
      if (not set.isLocalSpacetree(treeId)) {
        continue;
      }
      const auto& statistics = set.getGridStatistics(treeId);
      result.trees++;
      result.unrefinedCells += statistics.numberOfLocalUnrefinedCells;
      result.refinedCells += statistics.numberOfLocalRefinedCells;
      result.maxDepth = std::max(result.maxDepth, statistics.maxDepth);
      result.unrefinedCellsPerTree.push_back(statistics.numberOfLocalUnrefinedCells);
    }
    std::sort(result.unrefinedCellsPerTree.begin(), result.unrefinedCellsPerTree.end());

    result.routingResolved   = not set._periodicRouting.empty();
    result.routingIsSelfOnly = result.routingResolved;
    for (const auto& [key, sides] : set._periodicRouting) {
      for (const auto& trees : sides) {
        for (const int treeId : trees) {
          result.routingIsSelfOnly &= treeId == rootId;
        }
      }
    }

    // Remove every trace of the scenario, so subsequent scenarios and tests
    // find the set exactly as the harness initialised it.
    for (auto tree = set._spacetrees.begin(); tree != set._spacetrees.end();) {
      if (family.count(tree->getId()) > 0) {
        set.unregisterLocalSpacetree(tree->getId());
        node.deregisterId(tree->getId());
        tree = set._spacetrees.erase(tree);
      } else {
        ++tree;
      }
    }
  }

  set._periodicRouting.clear();
  set._crossRankPeriodicChannels.clear();
  set._lastPeriodicProducerDescriptors.clear();
  set._lastPeriodicConsumerDescriptors.clear();
  set._crossRankPeriodicGridVertexStaging.clear();
  set._periodicBC = backupPeriodicBC;

  return result;
}

void peano4::parallel::tests::SpacetreeSetTest::testRegularGridLoadBalancingMatchesNonPeriodicRun() {
  const auto withPeriodic    = runScenario(true, 4, false, false);
  const auto withoutPeriodic = runScenario(false, 4, false, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The load balancing input is periodic-agnostic, so the decomposition has
    // to coincide tree by tree.
    validateEquals(withPeriodic.trees, withoutPeriodic.trees);
    validateEquals(withPeriodic.unrefinedCells, withoutPeriodic.unrefinedCells);
    validateEquals(withPeriodic.refinedCells, withoutPeriodic.refinedCells);
    validateEquals(withPeriodic.maxDepth, withoutPeriodic.maxDepth);
    validateEquals(
      static_cast<int>(withPeriodic.unrefinedCellsPerTree.size()),
      static_cast<int>(withoutPeriodic.unrefinedCellsPerTree.size())
    );
    for (std::size_t i = 0; i < withPeriodic.unrefinedCellsPerTree.size(); i++) {
      validateEqualsWithParams1(
        withPeriodic.unrefinedCellsPerTree[i],
        withoutPeriodic.unrefinedCellsPerTree[i],
        static_cast<int>(i)
      );
    }

    validateEquals(withPeriodic.trees, 4);
    validate(withPeriodic.splitsSucceeded);
    validate(withoutPeriodic.splitsSucceeded);
    validate(withPeriodic.routingResolved);
    validate(not withoutPeriodic.routingResolved);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testCornerRefinementMirrorsAcrossPeriodicBoundaries() {
  const auto withPeriodic    = runScenario(true, 1, true, false);
  const auto withoutPeriodic = runScenario(false, 1, true, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The corner region straddles every periodic axis, so the wrap has to
    // mirror its refinement onto the far sides: the periodic grid holds
    // strictly more cells than the non-periodic one, whose refinement stays
    // confined to the corner.
    validateEquals(withPeriodic.maxDepth, withoutPeriodic.maxDepth);
    validateWithParams2(
      withPeriodic.unrefinedCells > withoutPeriodic.unrefinedCells,
      withPeriodic.unrefinedCells,
      withoutPeriodic.unrefinedCells
    );

    // A single tree owns both sides of every wrap, so the routing pairs the
    // tree with itself.
    validate(withPeriodic.routingResolved);
    validate(withPeriodic.routingIsSelfOnly);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testMultiTreePeriodicGridMatchesSingleTree() {
  const auto oneTree   = runScenario(true, 1, true, false);
  const auto fourTrees = runScenario(true, 4, true, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The grid, including the refinement mirrored across the wrap, must not
    // depend on the domain decomposition.
    validateEquals(fourTrees.trees, 4);
    validate(fourTrees.splitsSucceeded);
    validateEquals(fourTrees.unrefinedCells, oneTree.unrefinedCells);
    validateEquals(fourTrees.maxDepth, oneTree.maxDepth);
    validate(fourTrees.routingResolved);
    validate(not fourTrees.routingIsSelfOnly);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testEraseAtPeriodicBoundaryRestoresRegularGrid() {
  // Erasing runs on one tree: Peano vetoes coarsening above forked subtrees,
  // so a multi-tree grid deliberately keeps refinement deployed to workers,
  // with or without periodic boundaries.
  const auto regularGrid = runScenario(true, 1, false, false);
  const auto erasedAgain = runScenario(true, 1, true, true);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The erase has to travel across the wrap exactly like the refinement
    // did, so the mirrored bands disappear too and the regular grid returns.
    validateEquals(erasedAgain.trees, regularGrid.trees);
    validateEquals(erasedAgain.unrefinedCells, regularGrid.unrefinedCells);
    validateEquals(erasedAgain.maxDepth, regularGrid.maxDepth);
  }
}
