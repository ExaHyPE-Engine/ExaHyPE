#include "../strategies/SpreadOutOnceGridStagnates.h"

#include "toolbox/loadbalancing/loadbalancing.h"


#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log  toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::_log( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates" );


toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::SpreadOutOnceGridStagnates(Configuration* configuration, CostMetrics*   costMetrics ):
  AbstractLoadBalancing( configuration, costMetrics ),
  _previousNumberOfCells(-1),
  _numberOfStableGridIterations(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::~SpreadOutOnceGridStagnates() {
}



int toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::getNumberOfTreesPerRank() const {
  int result = 0;

  if (
    _state==State::InterRankDistribution
    and
    _statistics.hasConsistentViewOfWorld()
    and
    _numberOfStableGridIterations>3
  ) {
    int maxPermittedTreesPerRank = _configuration->getMaxLocalTreesPerRank(_state)>=1
                                 ? _configuration->getMaxLocalTreesPerRank(_state)
                                 : std::numeric_limits<int>::max();

    int treesPerRankToExploitAllThreads = std::min(
      maxPermittedTreesPerRank,
      tarch::multicore::Core::getInstance().getNumberOfThreads()
    );

    int minTreeSize = std::max(
      _configuration->getMinTreeSize(_state),
      1
    );

    int treesPerRankAccommodatingMinTreeSize = std::max(
      _statistics.getLocalNumberOfInnerUnrefinedCells() / minTreeSize / tarch::mpi::Rank::getInstance().getNumberOfRanks(),
      1
    );

    result = std::min( treesPerRankAccommodatingMinTreeSize, treesPerRankToExploitAllThreads );

    logInfo(
      "getNumberOfTreesPerRank()",
      "mesh has " << _statistics.getLocalNumberOfInnerUnrefinedCells() <<
      " cells, so split " << result << " times " <<
      "(maxPermittedTreesPerRank=" << maxPermittedTreesPerRank <<
      ",treesPerRankToExploitAllThreads=" << treesPerRankToExploitAllThreads <<
      ",minTreeSize=" << minTreeSize <<
      ",treesPerRankAccommodatingMinTreeSize" << treesPerRankAccommodatingMinTreeSize <<
      ")"
    );
  }

  return result;
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::updateLoadBalancing() {
  if (
    _statistics.hasConsistentViewOfWorld()
    and
    _previousNumberOfCells == _statistics.getGlobalNumberOfInnerUnrefinedCells()
  ) {
    _numberOfStableGridIterations++;
  }
  else {
    _numberOfStableGridIterations = 0;
  }
 
  if (
    not tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    _state == State::InterRankDistribution
  ) {
    _state = State::Stagnation;
  }
  
  const int numberOfTreesPerRank = getNumberOfTreesPerRank();

  if ( numberOfTreesPerRank>0 ) {
    const int ranks   = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    int cellsPerTree = std::max(
      static_cast<int>(std::round(_statistics.getGlobalNumberOfInnerUnrefinedCells() / ranks / numberOfTreesPerRank )),
      1
    );

    logInfo(
      "updateLoadBalancing()",
      "try to create " << numberOfTreesPerRank <<
      " trees per rank with internal state" << toString() <<
      " to produce " << ranks << "x" << numberOfTreesPerRank << 
      " trees with approx " << cellsPerTree << " cells per tree"
    );

    int totalSplits = cellsPerTree;
    for (int targetRank=0; targetRank<ranks;   targetRank++ )
    for (int treeNumber= targetRank==0 ? 1 : 0; treeNumber<numberOfTreesPerRank; treeNumber++ ) {
      int thisTreesCells = cellsPerTree;
      if (static_cast<int>(_statistics.getGlobalNumberOfInnerUnrefinedCells()) % (ranks*numberOfTreesPerRank) >= totalSplits) {
        thisTreesCells++;
      }

      triggerSplit(thisTreesCells, targetRank);

      totalSplits++;
    }

    _state = State::Stagnation;
  }
  else if (not _statistics.hasConsistentViewOfWorld()) {
    _previousNumberOfCells = -1;
  }
  else {
    _previousNumberOfCells = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  }
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange( _state );

  updateLoadBalancing();
  dumpStatistics();
}


void toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates::triggerSplit( int numberOfCells, int targetRank ) {
  logInfo( "triggerSplit(int,int,int)", "trigger split from tree 0 into new tree on rank " << targetRank << " with " << numberOfCells << " cell(s)" );

  assertionEquals( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1 );
  assertion( tarch::mpi::Rank::getInstance().isGlobalMaster() );

  const int sourceTree = 0;
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,peano4::SplitInstruction{numberOfCells,peano4::SplitInstruction::Mode::BottomUp},targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  _blacklist.triggeredSplit( sourceTree );
  _statistics.incLocalNumberOfSplits();
}


