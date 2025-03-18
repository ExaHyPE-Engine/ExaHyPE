// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpreadOutOnceGridStagnates.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"

tarch::logging::Log toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::_log(
  "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates"
);


toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::SpreadOutOnceGridStagnates(
  Configuration* configuration,
  CostMetrics*   costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics),
  _previousNumberOfCells(-1),
  _numberOfStableGridIterations(0) {

  assertion(configuration != nullptr);
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


int toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::getNumberOfTreesPerRank() const {
  int result = 0;

  if (_state == State::InterRankDistribution and _statistics.hasConsistentViewOfWorld() and _numberOfStableGridIterations > 3) {
    const int maxPermittedTreesPerRank = _configuration->getMaxLocalTreesPerRank(_state) >= 1
                                           ? _configuration->getMaxLocalTreesPerRank(_state)
                                           : std::numeric_limits<int>::max();

    const int treesPerRankToExploitAllThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();

    const int minTreeSize = std::max(_configuration->getMinTreeSize(_state), 1);

    const int treesPerRankAccommodatingMinTreeSize = std::max(
      _statistics.getLocalNumberOfInnerUnrefinedCells() / minTreeSize
        / tarch::mpi::Rank::getInstance().getNumberOfRanks(),
      1
    );

    result = std::min({maxPermittedTreesPerRank, treesPerRankAccommodatingMinTreeSize, treesPerRankToExploitAllThreads}
    );

    logInfo(
      "getNumberOfTreesPerRank()",
      "Mesh has "
        << _statistics.getLocalNumberOfInnerUnrefinedCells() << " cells, so split " << result << " times "
        << "(MaxPermittedTreesPerRank=" << maxPermittedTreesPerRank
        << ",TreesPerRankToExploitAllThreads=" << treesPerRankToExploitAllThreads << ",minTreeSize=" << minTreeSize
        << ",TreesPerRankAccommodatingMinTreeSize=" << treesPerRankAccommodatingMinTreeSize << ")"
    );
  }

  return result;
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::updateLoadBalancing() {
  if (_statistics.hasConsistentViewOfWorld() and _previousNumberOfCells == _statistics.getGlobalNumberOfInnerUnrefinedCells()) {
    _numberOfStableGridIterations++;
  } else {
    _numberOfStableGridIterations = 0;
  }

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster() and _state == State::InterRankDistribution) {
    _state = State::Stagnation;
  }

  const int numberOfTreesPerRank = getNumberOfTreesPerRank();

  if (numberOfTreesPerRank > 0) {
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    const int cellsPerTree = std::max(
      static_cast<int>(std::round(_statistics.getGlobalNumberOfInnerUnrefinedCells() / ranks / numberOfTreesPerRank)),
      1
    );

    int totalSplits = cellsPerTree;
    for (int targetRank = 0; targetRank < ranks; targetRank++) {
      for (int treeNumber = targetRank == 0 ? 1 : 0; treeNumber < numberOfTreesPerRank; treeNumber++) {
        int thisTreesCells = cellsPerTree;
        if (static_cast<int>(_statistics.getGlobalNumberOfInnerUnrefinedCells()) % (ranks * numberOfTreesPerRank) >= totalSplits) {
          thisTreesCells++;
        }

        triggerSplit(thisTreesCells, targetRank);

        totalSplits++;
      }
    }

    _state = State::Stagnation;
  } else if (not _statistics.hasConsistentViewOfWorld()) {
    _previousNumberOfCells = -1;
  } else {
    _previousNumberOfCells = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  }
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange(_state);

  updateLoadBalancing();
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::triggerSplit(int numberOfCells, int targetRank) {
  assertionEquals(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1);
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());

  const int  sourceTree = 0;
  const bool success    = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::SplitInstruction{numberOfCells, peano4::SplitInstruction::Mode::BottomUp},
    targetRank
  );

  if (not success) {
    logError("triggerSplit(...)", "Wanted to split local rank " << sourceTree << " but failed");
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}
