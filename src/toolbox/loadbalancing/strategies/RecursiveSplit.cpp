// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RecursiveSplit.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"

#include <cmath>


tarch::logging::Log toolbox::loadbalancing::strategies::RecursiveSplit::_log(
  "toolbox::loadbalancing::strategies::RecursiveSplit"
);


toolbox::loadbalancing::strategies::RecursiveSplit::RecursiveSplit(
  Configuration* configuration,
  CostMetrics*   costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics),
  _previousNumberOfCells(-1),
  _numberOfStableGridIterations(0),
  _numTrees(1),
  _hasRecentlySplit(false),
  _traversalsSinceSplit(0) {

  assertion(configuration != nullptr);
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


struct toolbox::loadbalancing::strategies::RecursiveSplit::SpacetreeComparator {
  bool operator()(const int& tree0, const int& tree1) const {
    return peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(tree0).getNumberOfLocalUnrefinedCells()
           > peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(tree1).getNumberOfLocalUnrefinedCells();
  }
};


int toolbox::loadbalancing::strategies::RecursiveSplit::getNumberOfRequiredSplits(int spacetreeId) const {
  int result = 0;

  int threshold = 27;

  // Adjust recursive factor depending on how the large tree is rather than
  // keeping it fixed.
  int recursiveFactor = 3;

  for (int i = 0; i < Dimensions - 1; i++) {
    threshold *= 3;
  }

  const int availableThreads = tarch::multicore::Core::getInstance().getNumberOfThreads() - _numTrees;

  // If a tree does not have the minimum number of required cells then do not
  // split into a recursiveFactor number of cells or there are not enough
  // threads to make new trees.

  if (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(spacetreeId).getNumberOfLocalUnrefinedCells() < threshold || availableThreads <= 0) {
    // 0 is returned to indicate no spacetree splitting should occur
    return result;
  }

  if (_state == State::InterRankDistribution and _statistics.hasConsistentViewOfWorld()) {
    if (_hasRecentlySplit) {
      int maxCellsPerTree = 0;

      for (int i = 0; i < _numTrees; i++) {
        maxCellsPerTree = std::max(
          maxCellsPerTree,
          peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(i).getNumberOfLocalUnrefinedCells()
        );
      }

      // If already split, then only split another tree if it also much more
      // than average number of cells in tree.
      int totalCells = 0;
      for (int i = 0; i < _numTrees; i++) {
        totalCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(i).getNumberOfLocalUnrefinedCells(
        );
      }

      int avgCellsPerTree = totalCells / (peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size());

      if (peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(spacetreeId).getNumberOfLocalUnrefinedCells() < avgCellsPerTree) {
        return result;
      }
    }

    // Calculate the recursive factor based on how large the tree is compared to
    // other trees, with a minimum recursive factor of 3.
    int totalCells = 0;

    for (int i = 0; i < peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(); i++) {
      totalCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(i).getNumberOfLocalUnrefinedCells();
    }

    int avgCellsPerTree = totalCells / (peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size());

    recursiveFactor = peano4::parallel::SpacetreeSet::getInstance()
                        .getGridStatistics(spacetreeId)
                        .getNumberOfLocalUnrefinedCells()
                      / avgCellsPerTree;

    recursiveFactor = std::max(recursiveFactor, 1);

    result = std::min(availableThreads, recursiveFactor);

    logInfo(
      "getNumberOfRequiredSplits()",
      "(tree #"
        << spacetreeId << " with "
        << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(spacetreeId).getNumberOfLocalUnrefinedCells()
        << " cells will split into " << result << " more trees.)"
    );
  }

  return result;
}


void toolbox::loadbalancing::strategies::RecursiveSplit::updateLoadBalancing() {
  // Remove idea of stable grid iterations

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster() and _state == State::InterRankDistribution) {
    _state = State::Stagnation;
  }

  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  std::set<int, SpacetreeComparator> sortedIdsOfLocalSpacetrees;

  for (auto spacetreeId : idsOfLocalSpacetrees) {
    sortedIdsOfLocalSpacetrees.insert(spacetreeId);
  }

  bool split      = false;
  int  splitCount = 0;

  if (_hasRecentlySplit == true && _traversalsSinceSplit <= 1) {
    _traversalsSinceSplit++;
  } else {
    _hasRecentlySplit     = false;
    _traversalsSinceSplit = 0;

    for (auto spacetreeId : sortedIdsOfLocalSpacetrees) {
      if (splitCount < 100) {

        int numberOfRequiredSplits = getNumberOfRequiredSplits(spacetreeId);

        if (numberOfRequiredSplits > 0) {
          split             = true;
          _hasRecentlySplit = true;

          const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

          int cellsPerTree = peano4::parallel::SpacetreeSet::getInstance()
                               .getGridStatistics(spacetreeId)
                               .getNumberOfLocalUnrefinedCells()
                             / (numberOfRequiredSplits + 1);

          logInfo(
            "updateLoadBalancing()",
            "split tree #"
              << spacetreeId << " to create " << numberOfRequiredSplits << " more trees"
              << " with approx " << cellsPerTree << " cells per tree"
          );

          for (int targetRank = 0; targetRank < ranks; targetRank++)
            for (int treeNumber = 0; treeNumber < numberOfRequiredSplits; treeNumber++) {
              triggerSplit(spacetreeId, cellsPerTree, targetRank);
            }

          splitCount++;
        }
      }
    }

    if (split == true) {
      _hasRecentlySplit = true;
    }
  }

  if (split == true && _numTrees >= tarch::multicore::Core::getInstance().getNumberOfThreads()) {
    _state = State::Stagnation;
  }
}


void toolbox::loadbalancing::strategies::RecursiveSplit::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange(_state);

  updateLoadBalancing();
}


void toolbox::loadbalancing::strategies::RecursiveSplit::
  triggerSplit(int sourceTree, int numberOfCells, int targetRank) {
  logInfo(
    "triggerSplit()",
    "trigger split from tree "
      << sourceTree << " into new tree on rank " << targetRank << " with " << numberOfCells << " cell(s)"
  );

  assertionEquals(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1);
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());

  bool success = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::SplitInstruction{numberOfCells, peano4::SplitInstruction::Mode::BottomUp},
    targetRank
  );

  if (not success) {
    logInfo("triggerSplit()", "wanted to split local rank " << sourceTree << " but failed");
  } else {
#pragma omp atomic
    _numTrees++;
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}
