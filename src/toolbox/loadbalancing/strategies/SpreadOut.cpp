// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpreadOut.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"

#include <cmath>

tarch::logging::Log toolbox::loadbalancing::strategies::SpreadOut::_log(
  "toolbox::loadbalancing::strategies::SpreadOut"
);


toolbox::loadbalancing::strategies::SpreadOut::SpreadOut(
  Configuration* configuration,
  CostMetrics*   costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics),
  _maxPermittedTreesPerRank(0) {

  assertion(configuration != nullptr);
  assertion(costMetrics != nullptr);

  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);

  _maxPermittedTreesPerRank = _configuration->getMaxLocalTreesPerRank(_state) >= 1 ? _configuration->getMaxLocalTreesPerRank(_state) : std::numeric_limits<int>::max();
}


void toolbox::loadbalancing::strategies::SpreadOut::updateInterRankLoadBalancing() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks <= 1 or _statistics.getGlobalNumberOfTrees() > 1) {
    _state = State::IntraRankDistribution; // Nothing to be done for inter rank load balancing
    return;
  }

  if (hasSplitRecently() or not _statistics.hasConsistentViewOfWorld() or not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return; // Nothing to do yet
  }

  const int numberOfCells = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  const int minTreeSize = std::max(_configuration->getMinTreeSize(_state), 1); // We want to split as early as possible here
  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / minTreeSize, 1);

  if (numberOfTreesToAccommodateMinTreeSize > numberOfRanks) {
    const int numberOfCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfRanks)), 1);
    logDebug(
      "updateInterRankLoadBalancing()",
      "Mesh has "
        << numberOfCells << " cells, so split " << numberOfRanks - 1 << " times "
        << "("
        << "MinTreeSize=" << minTreeSize
        << ", NumberOfTreesToAccommodateMinTreeSize=" << numberOfTreesToAccommodateMinTreeSize
        << ", NumberOfCellsPerTree=" << numberOfCellsPerTree
        << ")"
    );
    for (int targetRank = 1; targetRank < numberOfRanks; targetRank++ ) {
      int numberOfCellsPerTreeWithRemainder = numberOfCellsPerTree;
      if (numberOfCells % numberOfRanks >= targetRank) {
        numberOfCellsPerTreeWithRemainder++;
      }
      triggerSplit(numberOfCellsPerTreeWithRemainder, targetRank);
    }
  }
}


void toolbox::loadbalancing::strategies::SpreadOut::updateIntraRankLoadBalancing() {
  const int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();
  if (numberOfThreads <= 1 or peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 1) {
    _state = State::Stagnation;
    return;
  }

  constexpr int MinTreeSize = 1; // Using ThreePowerD leads to a better local cell distribution but a worse virtual cell distribution
  const int minTreeSize = std::max(_configuration->getMinTreeSize(_state), MinTreeSize);

  // const int heaviestSpacetree = getIdOfHeaviestLocalSpacetree();
  // if (heaviestSpacetree == NoHeaviestTreeAvailable and _blacklist.isBlacklisted(heaviestSpacetree)) {
  //   return;
  // }
  // const int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
  // const int numberOfCells = numberOfLocalUnrefinedCellsOfHeaviestSpacetree;
  const int numberOfCells = _statistics.getLocalNumberOfInnerUnrefinedCells();

  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / minTreeSize, 1);
  const int numberOfAllowedTrees = std::min({numberOfTreesToAccommodateMinTreeSize, _maxPermittedTreesPerRank, numberOfThreads});

  if (numberOfAllowedTrees < numberOfThreads or not _statistics.hasConsistentViewOfWorld()) {
    return; // Nothing to do (yet)
  }

  unsigned int numberOfSplits = numberOfAllowedTrees - 1;
  if (numberOfSplits > 0) {
    const int numberOfCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfAllowedTrees)), 1);
    logDebug(
      "updateIntraRankLoadBalancing()",
      "Mesh has "
        << numberOfCells << " cells, so split " << numberOfSplits << " times "
        << "("
        << "MinTreeSize=" << minTreeSize
        << ", MaxPermittedTreesPerRank=" << _maxPermittedTreesPerRank
        << ", NumberOfTreesToAccommodateMinTreeSize=" << numberOfTreesToAccommodateMinTreeSize
        << ", NumberOfCellsPerTree=" << numberOfCellsPerTree
        << ")"
    );
    unsigned int remainder = numberOfCells - (numberOfCellsPerTree * numberOfAllowedTrees);
    while (numberOfSplits) {
      if (remainder > 0) {
        triggerSplit(numberOfCellsPerTree + 1, tarch::mpi::Rank::getInstance().getRank());
        remainder--;
      } else {
        triggerSplit(numberOfCellsPerTree, tarch::mpi::Rank::getInstance().getRank());
      }
      numberOfSplits--;
    }
  }
}


void toolbox::loadbalancing::strategies::SpreadOut::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  // Start with InterRankDistribution
  logDebug("finishStep()", ::toString(_state));

  if (_state == State::InterRankDistribution) {
    updateInterRankLoadBalancing();
  } else if (_state == State::IntraRankDistribution) {
    updateIntraRankLoadBalancing();
  }

  _statistics.notifyOfStateChange(_state);
}


void toolbox::loadbalancing::strategies::SpreadOut::triggerSplit(int numberOfCells, int targetRank) {
  const int sourceTree = *(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin());
  const bool success   = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::SplitInstruction{numberOfCells, _configuration->getMode(_state)},
    targetRank
  );

  if (not success) {
    logError("triggerSplit()", "Wanted to split tree " << sourceTree << " but failed");
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}
