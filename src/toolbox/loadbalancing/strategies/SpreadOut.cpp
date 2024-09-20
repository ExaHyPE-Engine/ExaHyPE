#include "../strategies/SpreadOut.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"


tarch::logging::Log toolbox::loadbalancing::strategies::SpreadOut::_log("toolbox::loadbalancing::strategies::SpreadOut"
);


toolbox::loadbalancing::strategies::SpreadOut::SpreadOut(Configuration* configuration, CostMetrics* costMetrics):
  AbstractLoadBalancing(configuration, costMetrics),
  _previousNumberOfCells(-1),
  _numberOfStableGridIterations(0) {
  assertion(configuration != nullptr);
  assertion(costMetrics != nullptr);
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::strategies::SpreadOut::~SpreadOut() {}


int toolbox::loadbalancing::strategies::SpreadOut::getNumberOfTreesPerRank() const {
  int result = 0;

  if (_state == State::InterRankDistribution and _statistics.hasConsistentViewOfWorld()) {
    const int MaxTreesPerRank = std::min(
      _configuration->getMaxLocalTreesPerRank(_state), tarch::multicore::Core::getInstance().getNumberOfThreads()
    );
    const int MaxTotalTrees = MaxTreesPerRank * tarch::mpi::Rank::getInstance().getNumberOfRanks();

    const int MinMeshSizeToAccommodateMaxTotalTrees = _configuration->getMinTreeSize(_state) <= 1
                                                        ? MaxTotalTrees
                                                        : _configuration->getMinTreeSize(_state) * MaxTotalTrees;

    // Mesh is stable, so we should just split up as aggressively as possible,
    // even if we cannot keep all cores busy.
    if (_numberOfStableGridIterations > 3) {
      // min tree but at least give each rank something
      const int MinTreeSize = std::max(_configuration->getMinTreeSize(_state), 1);

      result = _statistics.getLocalNumberOfInnerUnrefinedCells() / MinTreeSize
               / tarch::mpi::Rank::getInstance().getNumberOfRanks();
      logInfo(
        "getNumberOfTreesPerRank()",
        "mesh is stable, so create "
          << result << " trees even though we might end up with fewer cells per tree than the lower bound "
          << _configuration->getMinTreeSize(_state) << " or might not be able to use all cores"
      );
    } else if (_statistics.getLocalNumberOfInnerUnrefinedCells() >= MinMeshSizeToAccommodateMaxTotalTrees) {
      result = MaxTotalTrees / tarch::mpi::Rank::getInstance().getNumberOfRanks();
      logInfo(
        "getNumberOfTreesPerRank()",
        "mesh has "
          << _statistics.getLocalNumberOfInnerUnrefinedCells() << " cells already, so create " << result
          << " trees per rank (min tree size=" << _configuration->getMinTreeSize(_state)
          << ", max-trees-per-rank=" << _configuration->getMaxLocalTreesPerRank(_state) << ")"
      );
    }
  }

  return result;
}


void toolbox::loadbalancing::strategies::SpreadOut::updateLoadBalancing() {
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

    int cellsPerTree = std::max(
      static_cast<int>(std::round(_statistics.getGlobalNumberOfInnerUnrefinedCells() / ranks / numberOfTreesPerRank)), 1
    );

    logInfo(
      "updateLoadBalancing()",
      "try to create "
        << numberOfTreesPerRank << " trees per rank with internal state" << toString() << " to produce " << ranks << "x"
        << numberOfTreesPerRank << " trees with approx " << cellsPerTree << " cells per tree"
    );

    int totalSplits = cellsPerTree;
    for (int targetRank = 0; targetRank < ranks; targetRank++)
      for (int treeNumber = targetRank == 0 ? 1 : 0; treeNumber < numberOfTreesPerRank; treeNumber++) {
        int thisTreesCells = cellsPerTree;
        if (static_cast<int>(_statistics.getGlobalNumberOfInnerUnrefinedCells()) % (ranks * numberOfTreesPerRank) >= totalSplits) {
          thisTreesCells++;
        }

        triggerSplit(thisTreesCells, targetRank);

        totalSplits++;
      }

    _state = State::Stagnation;
  } else if (not _statistics.hasConsistentViewOfWorld()) {
    _previousNumberOfCells = -1;
  } else {
    _previousNumberOfCells = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  }
}


void toolbox::loadbalancing::strategies::SpreadOut::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange(_state);

  updateLoadBalancing();
  dumpStatistics();
}


void toolbox::loadbalancing::strategies::SpreadOut::triggerSplit(int numberOfCells, int targetRank) {
  logInfo(
    "triggerSplit(int,int,int)",
    "trigger split from tree 0 into new tree on rank " << targetRank << " with " << numberOfCells << " cell(s)"
  );

  assertionEquals(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1);
  assertion(tarch::mpi::Rank::getInstance().isGlobalMaster());

  const int sourceTree = 0;
  bool      success    = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree, peano4::SplitInstruction{numberOfCells, _configuration->getMode(_state)}, targetRank
  );
  if (not success) {
    logInfo("triggerSplit()", "wanted to split local rank " << sourceTree << " but failed");
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}
