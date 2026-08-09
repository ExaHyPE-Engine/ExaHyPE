#include "toolbox/loadbalancing/strategies/Hardcoded.h"

#include "toolbox/loadbalancing/loadbalancing.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log  toolbox::loadbalancing::strategies::Hardcoded::_log( "toolbox::loadbalancing::strategies::Hardcoded" );


toolbox::loadbalancing::strategies::Hardcoded::Hardcoded(
  std::initializer_list<int> gridSweeps,
  std::initializer_list<int> splittingTrees,
  std::initializer_list<int> numberOfCells,
  std::initializer_list<int> destinationRanks,
  std::initializer_list<peano4::SplitInstruction::Mode> modes
):
  AbstractLoadBalancing( nullptr, nullptr ),
  _currentGridSweep(0) {
  assertionEquals4( gridSweeps.size(),       numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( splittingTrees.size(),   numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( destinationRanks.size(), numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( modes.size(),            numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );

  auto gridSweep       = gridSweeps.begin();
  auto splittingTree   = splittingTrees.begin();
  auto cells           = numberOfCells.begin();
  auto destinationRank = destinationRanks.begin();
  auto mode            = modes.begin();

  while (gridSweep != gridSweeps.end()) {
    _splits.push( Split( *gridSweep, *splittingTree, *cells, *destinationRank, *mode ) );
    gridSweep++;
    splittingTree++;
    cells++;
    destinationRank++;
    mode++;
  }
}


toolbox::loadbalancing::strategies::Hardcoded::Hardcoded(
  std::initializer_list<int> gridSweeps,
  std::initializer_list<int> splittingTrees,
  std::initializer_list<int> numberOfCells,
  std::initializer_list<int> destinationRanks,
  peano4::SplitInstruction::Mode mode
):
  AbstractLoadBalancing( nullptr, nullptr ),
  _currentGridSweep(0) {
  assertionEquals4( gridSweeps.size(),       numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( splittingTrees.size(),   numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );
  assertionEquals4( destinationRanks.size(), numberOfCells.size(), gridSweeps.size(), splittingTrees.size(), numberOfCells.size(), destinationRanks.size() );

  auto gridSweep       = gridSweeps.begin();
  auto splittingTree   = splittingTrees.begin();
  auto cells           = numberOfCells.begin();
  auto destinationRank = destinationRanks.begin();

  while (gridSweep != gridSweeps.end()) {
    _splits.push( Split( *gridSweep, *splittingTree, *cells, *destinationRank, mode ) );
    gridSweep++;
    splittingTree++;
    cells++;
    destinationRank++;
  }
}


void toolbox::loadbalancing::strategies::Hardcoded::Split::validateAndAmend() {
  #if defined(MPI)
  assertion1( tarch::mpi::Rank::getInstance().isInitialised(), toString() );

  const int MaxRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  #else
  const int MaxRanks = 1;
  #endif

  if ( destinationRank>=MaxRanks ) {
    int oldRank = destinationRank;
    int newRank = oldRank % tarch::mpi::Rank::getInstance().getNumberOfRanks();
    destinationRank = newRank;
    logWarning(
      "validateAndAddSplitInstruction(Split)",
      "split instruction tried to assign trees to rank " << oldRank <<
      " even though there are only " << tarch::mpi::Rank::getInstance().getNumberOfRanks() <<
      " ranks. Altered entry into " << toString()
    );
  }
}


toolbox::loadbalancing::strategies::Hardcoded::Split::Split( const Split& split ):
  gridSweep(split.gridSweep),
  splittingTree(split.splittingTree),
  numberOfCells(split.numberOfCells),
  destinationRank(split.destinationRank),
  mode(split.mode) {
}


toolbox::loadbalancing::strategies::Hardcoded::Split::Split( int gridSweep_,
  int splittingTree_,
  int numberOfCells_,
  int destinationRank_,
  peano4::SplitInstruction::Mode mode_
):
  gridSweep(gridSweep_),
  splittingTree(splittingTree_),
  numberOfCells(numberOfCells_),
  destinationRank(destinationRank_),
  mode(mode_) {
}


void toolbox::loadbalancing::strategies::Hardcoded::finishStep() {
  _currentGridSweep++;

  while (not _splits.empty() and _currentGridSweep >= _splits.front().gridSweep) {
    if (_currentGridSweep > _splits.front().gridSweep) {
      logError(
        "finishStep()",
        "splitting instructions lag behind actual simulation evolution (current grid sweep " 
        << _currentGridSweep << " while split should have happened in sweep " 
        << _splits.front().gridSweep << "). Try to catch up"
      );
    }

    Split split( _splits.front() );
    _splits.pop();

    split.validateAndAmend();

    if ( peano4::parallel::SpacetreeSet::getInstance().isLocalSpacetree( split.splittingTree) ) {
      if ( _state==State::SwitchedOff ) {
        logError(
          "finishStep()",
          "wanted to split " << split.toString() << ". However, load (re-)balancing is deactivated"
        );
      }
      else if (not peano4::parallel::SpacetreeSet::getInstance().split(
        split.splittingTree,
        peano4::SplitInstruction{split.numberOfCells,split.mode},
        split.destinationRank
      )) {
        logWarning(
          "finishStep()",
          "had told Peano to split " << split.toString() << ", but split failed"
        );
      }
    }
  }

  if (_splits.empty()) {
    logInfo(
      "finishStep()",
      "all split instructions have been handed out, so shutdown hardcoded load balancing now"
    );
    enable(false);
  }

  _statistics.updateGlobalView();
  _blacklist.update();

  dumpStatistics();
}


void toolbox::loadbalancing::strategies::Hardcoded::finishSimulation() {}


std::string toolbox::loadbalancing::strategies::Hardcoded::Split::toString() const {
  std::ostringstream msg;
  msg << "(gridSweep=" << gridSweep
      << ",splittingTree=" << splittingTree
      << ",numberOfCells=" << numberOfCells
      << ",destinationRank=" << destinationRank
      << ",mode=" << peano4::SplitInstruction::toString(mode)
      << ")";
  return msg.str();
}

