#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"


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
  _globalLightestRankRequest   = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfTreesRequest  = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  #endif
}


void toolbox::loadbalancing::Statistics::notifyOfStateChange( State state ) {
  _localLoadBalancingEnabled = state!=State::SwitchedOff;
}


std::string  toolbox::loadbalancing::Statistics::toString() const {
  std::ostringstream msg;

  msg << "(local-number-of-inner-unrefined-cells=" << _localNumberOfInnerUnrefinedCells
      << ",global-number-of-innter-unrefined-cells=" << _globalNumberOfInnerUnrefinedCells
      << ",global-number-of-trees=" << _globalNumberOfTrees
      << ",local-load-balancing-enabled=" << _localLoadBalancingEnabled
      << ",global-number-of-ranks-with-enabled-lb=" << _globalNumberOfRanksWithEnabledLoadBalancing
      << ",local-number-of-splits=" << _localNumberOfSplits
      << ",global-number-of-splits=" << _globalNumberOfSplits
      << ",number-of-state-updates-without-any-split=" << _numberOfStateUpdatesWithoutAnySplit
      << ",view-consistent=" << _viewConsistent;

  #if defined(Parallel)
  msg << ",lightest-rank-no=" << _lightestRank._rank
      << ",lightest-rank-weight=" << _lightestRank._unrefinedCells;
  #endif

  msg << ")";

  return msg.str();
}


void toolbox::loadbalancing::Statistics::waitForGlobalStatisticsExchange() {
  #ifdef Parallel
  if (_globalSumRequest != nullptr ) {
    MPI_Wait( _globalSumRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalLightestRankRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfSplitsRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfTreesRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfRanksWithEnabledLoadBalancingRequest, MPI_STATUS_IGNORE );

    delete _globalSumRequest;
    delete _globalLightestRankRequest;
    delete _globalNumberOfSplitsRequest;
    delete _globalNumberOfTreesRequest;
    delete _globalNumberOfRanksWithEnabledLoadBalancingRequest;

    _globalSumRequest            = nullptr;
    _globalLightestRankRequest   = nullptr;
    _globalNumberOfSplitsRequest = nullptr;
    _globalNumberOfTreesRequest  = nullptr;
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  }
  #endif
}


void toolbox::loadbalancing::Statistics::updateGlobalView() {
  _localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();

  _viewConsistent = true;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCells;
    _lightestRank._rank                          = 1;
    _lightestRank._unrefinedCells                = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _globalNumberOfRanksWithEnabledLoadBalancing = _localLoadBalancingEnabled ? 1 : 0;
  }
  else {
    #ifdef Parallel
    waitForGlobalStatisticsExchange();

    _globalNumberOfInnerUnrefinedCells = _globalNumberOfInnerUnrefinedCellsBufferIn;
    _lightestRank._rank                = _lightestRankIn._unrefinedCells < _localNumberOfInnerUnrefinedCells ? _lightestRankIn._rank : tarch::mpi::Rank::getInstance().getRank();
    _lightestRank._unrefinedCells      = _lightestRankIn._unrefinedCells;
    _globalNumberOfSplits              = _numberOfSplitsIn;
    _globalNumberOfTrees               = _numberOfTreesIn;
    _globalNumberOfRanksWithEnabledLoadBalancing = _numberOfRanksWithEnabledLoadBalancingIn;

    if ( _globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells ) {
      logInfo(
        "updateGlobalView()",
        "local number of cells (" << _localNumberOfInnerUnrefinedCells << ") is bigger than global cell count (" << _globalNumberOfInnerUnrefinedCells <<
        "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Postpone further load balancing decisions"
      );
      _viewConsistent = false;
      _globalNumberOfInnerUnrefinedCells       = _localNumberOfInnerUnrefinedCells;
      _lightestRank._rank                      = tarch::mpi::Rank::getInstance().getRank();
    }

    _globalSumRequest            = new MPI_Request();
    _globalLightestRankRequest   = new MPI_Request();
    _globalNumberOfSplitsRequest = new MPI_Request();
    _globalNumberOfTreesRequest  = new MPI_Request();
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = new MPI_Request();

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCells;
    _lightestRankOut._unrefinedCells                = _localNumberOfInnerUnrefinedCells;
    _lightestRankOut._rank                          = tarch::mpi::Rank::getInstance().getRank();
    _numberOfSplitsOut                              = _localNumberOfSplits;
    _numberOfTreesOut                               = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _numberOfRanksWithEnabledLoadBalancingOut       = _localLoadBalancingEnabled ? 1 : 0;

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
    MPI_Iallreduce(
      &_lightestRankOut,   // send
      &_lightestRankIn,    // receive
      1,                         // count
      MPI_DOUBLE_INT,
      MPI_MINLOC,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalLightestRankRequest
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


int toolbox::loadbalancing::Statistics::getNumberOfStateUpdatesWithoutAnySplit() const {
  return _numberOfStateUpdatesWithoutAnySplit;
}


int toolbox::loadbalancing::Statistics::getLightestRank() const {
  return _lightestRank._rank;
}




bool toolbox::loadbalancing::Statistics::hasConsistentViewOfWorld() const {
  return _viewConsistent;
}


