#include "../strategies/Hardcoded.h"

#include "toolbox/loadbalancing/loadbalancing.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log  toolbox::loadbalancing::strategies::Hardcoded::_log( "toolbox::loadbalancing::strategies::Hardcoded" );


toolbox::loadbalancing::strategies::Hardcoded::Hardcoded(
  std::initializer_list<int> timeStamps,
  std::initializer_list<int> splittingTrees,
  std::initializer_list<int> numberOfCells,
  std::initializer_list<int> destinationRanks,
  std::initializer_list<peano4::SplitInstruction::Mode> modes,
  bool handOutOnePartitionPerCore
):
  AbstractLoadBalancing( nullptr, nullptr ),
  _currentTimeStamp(0),
  _handOutOnePartitionPerCore(handOutOnePartitionPerCore) {
  assertionEquals4( timeStamps.size(),       numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( splittingTrees.size(),   numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( destinationRanks.size(), numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( modes.size(),            numberOfCells.size(), timeStamps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );

  auto timeStamp       = timeStamps.begin();
  auto splittingTree   = splittingTrees.begin();
  auto cells           = numberOfCells.begin();
  auto destinationRank = destinationRanks.begin();
  auto mode            = modes.begin();

  while (timeStamp!=timeStamps.end()) {
    _splits.push( Split( *timeStamp, *splittingTree, *cells, *destinationRank, *mode ) );
    timeStamp++;
    splittingTree++;
    cells++;
    destinationRank++;
    mode++;
  }

  // I would love to have some info output here, but some codes use the lb as static attribute
  // so it might come up before the logging infrastructure is up. This means that the logInfo
  // will crash.
  // logInfo( "Hardcoded(...)", "created hardcoded load balancing strategy with " << _splits.size() << " decomposition(s)" );
}



toolbox::loadbalancing::strategies::Hardcoded::Split::Split( const Split& split ):
  timeStamp( split.timeStamp ),
  splittingTree( split.splittingTree ),
  numberOfCells( split.numberOfCells ),
  destinationRank( split.destinationRank ),
  mode( split.mode ) {
}


toolbox::loadbalancing::strategies::Hardcoded::Split::Split( int  timeStamp_, int  splittingTree_, int  numberOfCells_, int  destinationRank_, peano4::SplitInstruction::Mode mode_ ):
  timeStamp( timeStamp_),
  splittingTree( splittingTree_),
  numberOfCells( numberOfCells_ ),
  destinationRank( destinationRank_ ),
  mode(mode_) {
}


void toolbox::loadbalancing::strategies::Hardcoded::finishStep() {
  _currentTimeStamp++;

  while (not _splits.empty() and _currentTimeStamp>=_splits.front().timeStamp) {
    Split split( _splits.front() );
    _splits.pop();
    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree( split.splittingTree) ) {
      if ( _state==State::SwitchedOff ) {
        logError(
          "finishStep()",
          "wanted to split " << split.numberOfCells << " cell(s) from tree " <<
          split.splittingTree << " and to deploy them to new tree on rank " << split.destinationRank <<
          ". However, load (re-)balancing is deactivated"
        );
      }
      else if (
        split.destinationRank == tarch::mpi::Rank::getInstance().getRank()
        and
        _handOutOnePartitionPerCore
        and
        static_cast<int>(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()) >= tarch::multicore::Core::getInstance().getNumberOfThreads()
      ) {
        logWarning(
          "finishStep()", "ignore instruction to split rank further by  cutting of " << split.numberOfCells << 
          ", as lb is told not to overbook local cores"
        );
      }
      else if (not peano4::parallel::SpacetreeSet::getInstance().split(
        split.splittingTree,
        peano4::SplitInstruction{split.numberOfCells,split.mode},
        split.destinationRank
      )) {
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


void toolbox::loadbalancing::strategies::Hardcoded::finishSimulation() {}
