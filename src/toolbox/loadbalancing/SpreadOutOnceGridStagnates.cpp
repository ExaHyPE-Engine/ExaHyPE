#include "SpreadOutOnceGridStagnates.h"
#include "loadbalancing.h"


#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log  toolbox::loadbalancing::SpreadOutOnceGridStagnates::_log( "toolbox::loadbalancing::SpreadOutOnceGridStagnates" );


toolbox::loadbalancing::SpreadOutOnceGridStagnates::SpreadOutOnceGridStagnates(Configuration* configuration, bool deleteInDestructor):
  AbstractLoadBalancing( configuration, deleteInDestructor ),
  _previousNumberOfCells(-1),
  _numberOfStableGridIterations(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::SpreadOutOnceGridStagnates::~SpreadOutOnceGridStagnates() {
}



int toolbox::loadbalancing::SpreadOutOnceGridStagnates::getNumberOfTreesPerRank() const {
  int result = 0;

  if (
    _state==State::InterRankDistribution
    and
    _statistics.hasConsistentViewOfWorld()
    and
    _numberOfStableGridIterations>3
  ) {
    const int MaxTreesPerRank = std::min(
      _configuration->getMaxLocalTreesPerRank(_state),
      tarch::multicore::Core::getInstance().getNumberOfThreads()
    );
    const int MaxTotalTrees = MaxTreesPerRank * tarch::mpi::Rank::getInstance().getNumberOfRanks();

    const int MinMeshSizeToAccommodateMaxTotalTrees = _configuration->getMinTreeSize(_state)<=1 ? MaxTotalTrees : _configuration->getMinTreeSize(_state) * MaxTotalTrees;
  
    const int MinTreeSize = std::max(
      _configuration->getMinTreeSize(_state),
      1
    );

    result = std::min(
      MaxTreesPerRank,
      std::max(1,_statistics.getLocalNumberOfInnerUnrefinedCells() / MinTreeSize / tarch::mpi::Rank::getInstance().getNumberOfRanks())
    );

    logInfo(
      "getNumberOfTreesPerRank()",
      "mesh has " << _statistics.getLocalNumberOfInnerUnrefinedCells() <<
      " cells already (we would need at least " << MinMeshSizeToAccommodateMaxTotalTrees <<
      " cells)"
    );
    logInfo(
      "getNumberOfTreesPerRank()",
      "create " << result << " trees per rank to obtain up to " << MaxTotalTrees << " trees in total (min tree size=" << _configuration->getMinTreeSize(_state) <<
      ", max-trees-per-rank=" << _configuration->getMaxLocalTreesPerRank(_state) << ")"
    );
  }

  return result;
}


void toolbox::loadbalancing::SpreadOutOnceGridStagnates::updateLoadBalancing() {
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


void toolbox::loadbalancing::SpreadOutOnceGridStagnates::finishStep() {
  _statistics.updateGlobalView();
  _blacklist.update();

  _statistics.notifyOfStateChange( _state );

  updateLoadBalancing();
  dumpStatistics();
}


void toolbox::loadbalancing::SpreadOutOnceGridStagnates::triggerSplit( int numberOfCells, int targetRank ) {
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


