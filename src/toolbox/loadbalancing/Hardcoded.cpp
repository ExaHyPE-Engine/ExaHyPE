#include "Hardcoded.h"
#include "loadbalancing.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log  toolbox::loadbalancing::Hardcoded::_log( "toolbox::loadbalancing::Hardcoded" );


toolbox::loadbalancing::Hardcoded::Hardcoded(std::initializer_list<int> timeStamps, std::initializer_list<int> splittingTrees, std::initializer_list<int> numberOfCells,  std::initializer_list<int> destinationRanks):
  _currentTimeStamp(0),
  _enabled(true) {
  assertionEquals4( timeStamps.size(),       numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( splittingTrees.size(),   numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( destinationRanks.size(), numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );

  auto timeStamp       = timeStamps.begin();
  auto splittingTree   = splittingTrees.begin();
  auto cells           = numberOfCells.begin();
  auto destinationRank = destinationRanks.begin();

  while (timeStamp!=timeStamps.end()) {
    _splits.push( Split( *timeStamp, *splittingTree, *cells, *destinationRank ) );
    timeStamp++;
    splittingTree++;
    cells++;
    destinationRank++;
  }

  logInfo( "Hardcoded(...)", "created hardcoded load balancing strategy with " << _splits.size() << " decomposition(s)" );
}



toolbox::loadbalancing::Hardcoded::Split::Split( const Split& split ):
  timeStamp( split.timeStamp ),
  splittingTree( split.splittingTree ),
  numberOfCells( split.numberOfCells ),
  destinationRank( split.destinationRank ) {
}


toolbox::loadbalancing::Hardcoded::Split::Split( int  timeStamp_, int  splittingTree_, int  numberOfCells_, int  destinationRank_ ):
  timeStamp( timeStamp_),
  splittingTree( splittingTree_),
  numberOfCells( numberOfCells_ ),
  destinationRank( destinationRank_ ) {
}


void toolbox::loadbalancing::Hardcoded::finishStep() {
  _currentTimeStamp++;

  while (not _splits.empty() and _currentTimeStamp>=_splits.front().timeStamp) {
    Split split( _splits.front() );
    _splits.pop();
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree( split.splittingTree) ) {
      if (not _enabled) {
        logError(
          "finishStep()",
          "wanted to split " << split.numberOfCells << " cell(s) from tree " <<
          split.splittingTree << " and to deploy them to new tree on rank " << split.destinationRank <<
          ". However, load (re-)balancing is deactivated"
        );
      }
      else if (not peano4::parallel::SpacetreeSet::getInstance().split(split.splittingTree,split.numberOfCells,split.destinationRank)) {
        logWarning(
          "finishStep()",
          "had been told to split " << split.numberOfCells << " cell(s) from tree " <<
          split.splittingTree << " and to deploy them to new tree on rank " << split.destinationRank <<
          ". However, that failed"
        );
      }
    }
  }

  dumpStatistics();
}


void toolbox::loadbalancing::Hardcoded::dumpStatistics() {
}


void toolbox::loadbalancing::Hardcoded::finishSimulation() {
}


bool toolbox::loadbalancing::Hardcoded::hasSplitRecently() const {
  return not _splits.empty();
}


void toolbox::loadbalancing::Hardcoded::enable(bool value) {
  _enabled = value;
}
