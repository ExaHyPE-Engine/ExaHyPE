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
  _minTreeSize(1),
  // _minTreeSize(ThreePowerD), // Using ThreePowerD leads to a better local cell distribution but a worse virtual cell distribution
  _maxPermittedTreesPerRank(0) {

  assertion(configuration != nullptr);
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);

  _minTreeSize = std::max(_configuration->getMinTreeSize(_state), _minTreeSize);
  // TODO: Not yet considered in this strategy
  _maxPermittedTreesPerRank = _configuration->getMaxLocalTreesPerRank(_state) >= 1 ? _configuration->getMaxLocalTreesPerRank(_state) : std::numeric_limits<int>::max();
}


void toolbox::loadbalancing::strategies::SpreadOut::updateLoadBalancing() {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster() and _state == State::InterRankDistribution) {
    _state = State::Stagnation;
  }

  const int numberOfCells = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / _minTreeSize, 1);
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int numberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();
  const int numberOfTreesToExploitAllCores = numberOfThreads * numberOfRanks;

  if (numberOfTreesToAccommodateMinTreeSize < numberOfTreesToExploitAllCores) {
    return;
    // TODO: If we cannot satisfy all cores, then we skip the splitting and try it next time again (default behaviour).
    // The user's choice of the min tree size should always be respected.
  }

  const int numberOfAllowedTrees = std::min({numberOfTreesToAccommodateMinTreeSize, numberOfTreesToExploitAllCores});
  unsigned int numberOfSplits = numberOfAllowedTrees - 1;

  if (_state == State::InterRankDistribution and _statistics.hasConsistentViewOfWorld() and numberOfSplits > 0) {
    const int numberOfCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfAllowedTrees)), 1);

    logInfo(
      "updateLoadBalancing()",
      "Mesh has "
        << numberOfCells << " cells, so split " << numberOfSplits << " times "
        << "("
        << "MinTreeSize=" << _minTreeSize
        << ", MaxPermittedTreesPerRank=" << _maxPermittedTreesPerRank
        << ", NumberOfTreesToExploitAllCores=" << numberOfTreesToExploitAllCores
        << ", NumberOfTreesToAccommodateMinTreeSize=" << numberOfTreesToAccommodateMinTreeSize
        << ", NumberOfCellsPerTree=" << numberOfCellsPerTree
        << ")"
    );

    unsigned int remainder = numberOfCells - (numberOfCellsPerTree * numberOfAllowedTrees);
    unsigned int currentTargetRank = numberOfRanks - 1;

    while (numberOfSplits) {
      if (remainder > 0) {
        triggerSplit(numberOfCellsPerTree + 1, currentTargetRank);
        remainder--;
      } else {
        triggerSplit(numberOfCellsPerTree, currentTargetRank);
      }
      numberOfSplits--;
      // Update the target rank in a round-robin manner
      currentTargetRank = (currentTargetRank - 1 + numberOfRanks) % numberOfRanks;
    }

    _state = State::Stagnation;
  }
}


void toolbox::loadbalancing::strategies::SpreadOut::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange(_state);

  updateLoadBalancing();
}


void toolbox::loadbalancing::strategies::SpreadOut::triggerSplit(int numberOfCells, int targetRank) {
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());
  assertionEquals(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1);

  const int  sourceTree = 0;
  const bool success    = peano4::parallel::SpacetreeSet::getInstance().split(
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
