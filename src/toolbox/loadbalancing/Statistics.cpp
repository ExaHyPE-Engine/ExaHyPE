#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"


tarch::logging::Log  toolbox::loadbalancing::Statistics::_log( "toolbox::loadbalancing::Statistics" );


toolbox::loadbalancing::Statistics::Statistics():
  _localNumberOfInnerUnrefinedCells( 0 ),
  _globalNumberOfInnerUnrefinedCells( 0 ),
  _globalNumberOfTrees(1),
  _globalNumberOfRanksWithEnabledLoadBalancing(0),
  _localLoadBalancingEnabled(true),
  _localNumberOfSplits(0),
  _numberOfStateUpdatesWithoutAnySplit(0),
  _viewConsistent(false) {
  #ifdef Parallel
  _globalSumRequest            = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfTreesRequest  = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  _globalMaximumTreesPerRankRequest                   = nullptr;
  #endif
}


void toolbox::loadbalancing::Statistics::notifyOfStateChange( State state ) {
  _localLoadBalancingEnabled = state!=State::SwitchedOff;
}


std::string  toolbox::loadbalancing::Statistics::toString() const {
  std::ostringstream msg;

  msg << "(local-number-of-inner-unrefined-cells=" << _localNumberOfInnerUnrefinedCells
      << ",global-number-of-inner-unrefined-cells=" << _globalNumberOfInnerUnrefinedCells
      << ",global-number-of-trees=" << _globalNumberOfTrees
      << ",max-trees-per-rank=" << _maximumTreesPerRank
      << ",local-load-balancing-enabled=" << _localLoadBalancingEnabled
      << ",global-number-of-ranks-with-enabled-lb=" << _globalNumberOfRanksWithEnabledLoadBalancing
      << ",local-number-of-splits=" << _localNumberOfSplits
      << ",global-number-of-splits=" << _globalNumberOfSplits
      << ",number-of-state-updates-without-any-split=" << _numberOfStateUpdatesWithoutAnySplit
      << ",view-consistent=" << _viewConsistent
      << ")";

  return msg.str();
}


void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
  #ifdef Parallel
  if (_globalSumRequest != nullptr ) {
    MPI_Wait( _globalSumRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfSplitsRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfTreesRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfRanksWithEnabledLoadBalancingRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalMaximumTreesPerRankRequest, MPI_STATUS_IGNORE );

    delete _globalSumRequest;
    delete _globalNumberOfSplitsRequest;
    delete _globalNumberOfTreesRequest;
    delete _globalNumberOfRanksWithEnabledLoadBalancingRequest;
    delete _globalMaximumTreesPerRankRequest;

    _globalSumRequest            = nullptr;
    _globalNumberOfSplitsRequest = nullptr;
    _globalNumberOfTreesRequest  = nullptr;
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
    _globalMaximumTreesPerRankRequest          = nullptr;
  }
  #endif
}


void toolbox::loadbalancing::Statistics::updateGlobalView() {
  _localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();

  _viewConsistent = true;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _globalNumberOfRanksWithEnabledLoadBalancing = _localLoadBalancingEnabled ? 1 : 0;
    _maximumTreesPerRank                         = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
  }
  else {
    #ifdef Parallel
    waitForGlobalDataExchange();

    _globalNumberOfInnerUnrefinedCells = _globalNumberOfInnerUnrefinedCellsBufferIn;
    _globalNumberOfSplits              = _numberOfSplitsIn;
    _globalNumberOfTrees               = _numberOfTreesIn;
    _globalNumberOfRanksWithEnabledLoadBalancing = _numberOfRanksWithEnabledLoadBalancingIn;
    _maximumTreesPerRank                         = _maximumTreesIn;

    if ( _globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells ) {
      logInfo(
        "updateGlobalView()",
        "local number of cells (" << _localNumberOfInnerUnrefinedCells << ") is bigger than global cell count (" << _globalNumberOfInnerUnrefinedCells <<
        "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Statistics might have inconsistent view of world"
      );
      _viewConsistent = false;
      _globalNumberOfInnerUnrefinedCells       = _localNumberOfInnerUnrefinedCells;
    }

    _globalSumRequest            = new MPI_Request();
    _globalNumberOfSplitsRequest = new MPI_Request();
    _globalNumberOfTreesRequest  = new MPI_Request();
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = new MPI_Request();
    _globalMaximumTreesPerRankRequest                   = new MPI_Request();

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCells;
    _numberOfSplitsOut                              = _localNumberOfSplits;
    _numberOfTreesOut                               = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _numberOfRanksWithEnabledLoadBalancingOut       = _localLoadBalancingEnabled ? 1 : 0;
    _numberOfMaximumTreesOut                        = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();

    MPI_Iallreduce(
      &_numberOfRanksWithEnabledLoadBalancingOut,  // send
      &_numberOfRanksWithEnabledLoadBalancingIn,   // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfRanksWithEnabledLoadBalancingRequest
    );
    MPI_Iallreduce(
      &_numberOfTreesOut,             // send
      &_numberOfTreesIn,              // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfTreesRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfInnerUnrefinedCellsBufferOut,  // send
      &_globalNumberOfInnerUnrefinedCellsBufferIn,   // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalSumRequest
    );
    // has to be global number, as local is already erased
    MPI_Iallreduce(
      &_numberOfSplitsOut,     // send
      &_numberOfSplitsIn,      // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfSplitsRequest
    );
    MPI_Iallreduce(
      &_numberOfMaximumTreesOut,     // send
      &_maximumTreesIn,      // receive
      1,             // count
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMaximumTreesPerRankRequest
    );
    #endif
  }

  if ( _globalNumberOfSplits==0 and _localNumberOfSplits==0 and _numberOfStateUpdatesWithoutAnySplit<65536) {
    _numberOfStateUpdatesWithoutAnySplit++;
  }
  else if ( _globalNumberOfSplits>0 or _localNumberOfSplits>0 ) {
    _numberOfStateUpdatesWithoutAnySplit = 0;
  }

  _localNumberOfSplits   = 0;
}


void toolbox::loadbalancing::Statistics::incLocalNumberOfSplits( int delta ) {
  assertion( delta>0 );
  _localNumberOfSplits += delta;
}


int toolbox::loadbalancing::Statistics::getLocalNumberOfInnerUnrefinedCells() const {
  return _localNumberOfInnerUnrefinedCells;
}


int toolbox::loadbalancing::Statistics::getGlobalNumberOfInnerUnrefinedCells() const {
  return _globalNumberOfInnerUnrefinedCells;
}


int toolbox::loadbalancing::Statistics::getGlobalNumberOfRanksWithEnabledLoadBalancing() const {
  return _globalNumberOfRanksWithEnabledLoadBalancing;
}


int toolbox::loadbalancing::Statistics::getGlobalNumberOfTrees() const {
  return _globalNumberOfTrees;
}


int toolbox::loadbalancing::Statistics::getMaximumTreesPerRank() const {
  return _maximumTreesPerRank;
}


int toolbox::loadbalancing::Statistics::getNumberOfStateUpdatesWithoutAnySplit() const {
  return _numberOfStateUpdatesWithoutAnySplit;
}


bool toolbox::loadbalancing::Statistics::hasConsistentViewOfWorld() const {
  return _viewConsistent;
}


