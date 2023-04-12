#include "SpreadOut.h"

tarch::logging::Log  toolbox::loadbalancing::SpreadOut::_log( "toolbox::loadbalancing::SpreadOut" );


toolbox::loadbalancing::SpreadOut::SpreadOut(Configuration* configuration):
  LoadBalancing( configuration ),
  _stepsToWaitForNextLoadBalancingDecision(0) {
  _state = State::InterRankDistribution;
}


toolbox::loadbalancing::SpreadOut::~SpreadOut() {
}


void toolbox::loadbalancing::SpreadOut::updateState() {
  if (
    _state==State::InterRankDistribution
    and
    _statistics.getGlobalNumberOfTrees() > 1
  ) {
    _state = State::Stagnation;
    // @todo Debug
    logInfo( "updateState()", "switch to state " << ::toString(_state) );
  }
}


int toolbox::loadbalancing::SpreadOut::getNumberOfTreesPerRank() const {
  int result = 0;

  if (
    _state==State::InterRankDistribution
    and
    _statistics.hasConsistentViewOfWorld()
  ) {
    const int MaxTotalTrees = std::max(
      _configuration->getMaxLocalTreesPerRank(_state) * tarch::mpi::Rank::getInstance().getNumberOfRanks(),
      tarch::mpi::Rank::getInstance().getNumberOfRanks()
    );
    const int MinTreeSize = std::max(
      _configuration->getMinTreeSize(_state) * MaxTotalTrees,
      tarch::mpi::Rank::getInstance().getNumberOfRanks()
    );

    if ( _statistics.getLocalNumberOfInnerUnrefinedCells() > MinTreeSize ) {
      result = MaxTotalTrees / tarch::mpi::Rank::getInstance().getNumberOfRanks();
    }
  }

  return result;
}


void toolbox::loadbalancing::RecursiveBipartition::updateLoadBalancing() {
  if ( getNumberOfTreesPerRank()>0 ) {
    logInfo( "updateLoadBalancing()", "try to create " << getNumberOfTreesPerRank() << " per rank with internal state" << toString() );

    const int ranks   = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    int cellsPerTree = std::max(
      static_cast<int>(std::round(_statistics.getGlobalNumberOfInnerUnrefinedCells() / ranks / getNumberOfTreesPerRank() )),
      1
    );

    int totalSplits = 0;
    for (int targetRank=0; targetRank<ranks;   targetRank++ )
    for (int treeNumber= targetRank==0 ? 1 : 0; treeNumber<getNumberOfTreesPerRank(); treeNumber++ ) {
      int thisTreesCells = cellsPerTree;
      if (static_cast<int>(_statistics.getGlobalNumberOfInnerUnrefinedCells()) % (ranks*getNumberOfTreesPerRank()) >= totalSplits) {
        thisRanksCells++;
      }

      triggerSplit(0, thisRanksCells, targetRank);

      totalSplits++;
    }
  }
}


void toolbox::loadbalancing::RecursiveBipartition::finishStep() {
  _statistics.updateGlobalView();
  _blacklist.update();

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange( _state );

  dumpStatistics();
}


void toolbox::loadbalancing::RecursiveBipartition::triggerSplit( int sourceTree, int numberOfCells, int targetRank ) {
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,numberOfCells,targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  _blacklist.triggeredSplit( sourceTree );
  _statistics.incLocalNumberOfSplits();
}


