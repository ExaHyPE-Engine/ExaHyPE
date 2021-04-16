#include "RecursiveSubdivision.h"
#include "loadbalancing.h"

#include "peano4/utils/Globals.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tarch.h"
#include "tarch/multicore/Core.h"

#include "peano4/grid/GridStatistics.h"




tarch::logging::Log  toolbox::loadbalancing::RecursiveSubdivision::_log( "toolbox::loadbalancing::RecursiveSubdivision" );


toolbox::loadbalancing::RecursiveSubdivision::RecursiveSubdivision(double targetBalancingRatio):
  _TargetBalancingRatio( targetBalancingRatio ),
  _blacklist(),
  _hasSpreadOutOverAllRanks( false ),
  _localNumberOfInnerUnrefinedCell( 0 ),
  _globalNumberOfInnerUnrefinedCells( 0 ),
  _globalNumberOfTrees(1),
  _globalNumberOfRanksWithEnabledLoadBalancing(0),
  _localNumberOfSplits(0),
  _numberOfStateUpdatesWithoutAnySplit(0),
  _state( StrategyState::Standard ),
  _enabled(true),
  _roundRobinToken(0),
  _maxTreeWeightAtLastSplit( std::numeric_limits<int>::max() ) 
{
  #ifdef Parallel
  _globalSumRequest            = nullptr;
  _globalLightestRankRequest   = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfTreesRequest  = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  #endif
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString() const {
  std::ostringstream msg;

  msg << "(state=" << toString( _state ) 
      << ",enabled=" << _enabled
      << ",global-cell-count=" << _globalNumberOfInnerUnrefinedCells 
      << ",lightest-rank-no=" << _lightestRank._rank
      << ",lightest-rank-weight=" << _lightestRank._unrefinedCells
      << ",has-spread-over-all-ranks=" << _hasSpreadOutOverAllRanks 
      << ",round-robin-token=" << _roundRobinToken 
      << ",target-balancing-ratio=" << _TargetBalancingRatio
      << ",max-tree-weight-at-last-split=" << _maxTreeWeightAtLastSplit
      << ",number-of-state-updated-without-any-split=" << _numberOfStateUpdatesWithoutAnySplit
      << ",global-number-of-splits=" << _globalNumberOfSplits
      << ",local-number-of-splits=" << _localNumberOfSplits
      << ",global-number-of-trees=" << _globalNumberOfTrees
      << ",global-number-of-ranks-with-enabled-lb=" << _globalNumberOfRanksWithEnabledLoadBalancing;

  if (_blacklist.empty()) {
    msg << ",blacklist is empty";
  }
  else {
    for (auto p: _blacklist) {
      msg << ",(#" << p.first << ":" << p.second << ")";
    }
  }

  msg << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
      << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
      << ",enabled=" <<  _enabled
      << ")";

  return msg.str();
}


toolbox::loadbalancing::RecursiveSubdivision::~RecursiveSubdivision() {
}


void toolbox::loadbalancing::RecursiveSubdivision::finishSimulation() {
  waitForGlobalStatisticsExchange();
}


void toolbox::loadbalancing::RecursiveSubdivision::waitForGlobalStatisticsExchange() {
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


void toolbox::loadbalancing::RecursiveSubdivision::updateGlobalView() {
  _localNumberOfInnerUnrefinedCell = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();

  if (
    not tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    not _hasSpreadOutOverAllRanks
  ) {
    _hasSpreadOutOverAllRanks = true;
    _roundRobinToken          = 0;
  }

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCell;
    _lightestRank._rank                          = 1;
    _lightestRank._unrefinedCells                = _localNumberOfInnerUnrefinedCell;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _globalNumberOfRanksWithEnabledLoadBalancing = _enabled ? 1 : 0;;
  }
  else {
    #ifdef Parallel
    waitForGlobalStatisticsExchange();

    _globalNumberOfInnerUnrefinedCells = _globalNumberOfInnerUnrefinedCellsBufferIn;
    _lightestRank._rank                = _lightestRankIn._unrefinedCells < _localNumberOfInnerUnrefinedCell ? _lightestRankIn._rank : tarch::mpi::Rank::getInstance().getRank();
    _lightestRank._unrefinedCells      = _lightestRankIn._unrefinedCells;
    _globalNumberOfSplits              = _numberOfSplitsIn;
    _globalNumberOfTrees               = _numberOfTreesIn;
    _globalNumberOfRanksWithEnabledLoadBalancing = _numberOfRanksWithEnabledLoadBalancingIn;

    if (
      _globalNumberOfInnerUnrefinedCells <= _localNumberOfInnerUnrefinedCell
      and
      not _hasSpreadOutOverAllRanks
    ) {
      logInfo(
        "updateGlobalView()",
        "global number of cells lags behind local one. Use local number of cells (" << _localNumberOfInnerUnrefinedCell <<
        ") instead of global count of " << _globalNumberOfInnerUnrefinedCells << " to guide partitioning"
      );
      _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCell;
      _lightestRank._rank                = tarch::mpi::Rank::getInstance().getRank();
    }
    else if (_globalNumberOfInnerUnrefinedCells <= _localNumberOfInnerUnrefinedCell ) {
      _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCell * tarch::mpi::Rank::getInstance().getNumberOfRanks();
      logInfo(
        "updateGlobalView()",
        "global number of cells lags behind local one. Code has spread over ranks already. Therefore, assume that we use global data from previous time step. Extrapolate " << _localNumberOfInnerUnrefinedCell 
        << " to guide partitioning. Assume it equals " << _globalNumberOfInnerUnrefinedCells
      );
      _lightestRank._rank        = tarch::mpi::Rank::getInstance().getRank();
    }

    _globalSumRequest            = new MPI_Request();
    _globalLightestRankRequest   = new MPI_Request();
    _globalNumberOfSplitsRequest = new MPI_Request();
    _globalNumberOfTreesRequest  = new MPI_Request();
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = new MPI_Request();

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCell;
    _lightestRankOut._unrefinedCells                = _localNumberOfInnerUnrefinedCell;
    _lightestRankOut._rank                          = tarch::mpi::Rank::getInstance().getRank();
    _numberOfSplitsOut                              = _localNumberOfSplits;
    _numberOfTreesOut                               = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _numberOfRanksWithEnabledLoadBalancingOut       = _enabled ? 1 : 0;

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


bool toolbox::loadbalancing::RecursiveSubdivision::doesRankViolateBalancingCondition() const {
  int localCells = 0;
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  for (auto p: idsOfLocalSpacetrees) {
    localCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
  }

  const double threshold = static_cast<double>(_globalNumberOfInnerUnrefinedCells)
                         / tarch::mpi::Rank::getInstance().getNumberOfRanks();

  const double illbalancing = (static_cast<double>(localCells)-threshold) / threshold;
  bool result = illbalancing > 1.0 - _TargetBalancingRatio;

  if (result) {
    logInfo( "doesRankViolateBalancingCondition()", "rank does violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _globalNumberOfInnerUnrefinedCells << ", local cells=" << localCells << ", threshold=" << threshold << ", balancing-ratio=" << _TargetBalancingRatio << ")" );
  }
  else {
    logDebug( "doesRankViolateBalancingCondition()", "rank does not violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _globalNumberOfInnerUnrefinedCells << ", local cells=" << localCells << ", threshold=" << threshold << ", balancing-ratio=" << _TargetBalancingRatio << ")" );
  }

  return result;
}


void toolbox::loadbalancing::RecursiveSubdivision::updateState() {
  _roundRobinToken++;
  _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();

  if (
    _localNumberOfInnerUnrefinedCell
    <
    std::max( 
      tarch::multicore::Core::getInstance().getNumberOfThreads(),
      tarch::mpi::Rank::getInstance().getNumberOfRanks()
    )
  ) {
    _state = StrategyState::PostponedDecisionDueToLackOfCells;
  }
  else if (
    _roundRobinToken!=tarch::mpi::Rank::getInstance().getRank()
  ) {
    _state = StrategyState::WaitForRoundRobinToken;
  }
  else if (
    getWeightOfHeaviestLocalSpacetree()==-1
  ) {
    _state = StrategyState::Stagnation;
  }
  else {
    _state = StrategyState::Standard;
  }
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString( StrategyStep step ) {
  switch ( step ) {
    case StrategyStep::Wait:
      return "wait";
    case StrategyStep::SpreadEquallyOverAllRanks:
      return "spread-equally-over-all-ranks";
    case StrategyStep::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(multiple-times,use-local-rank,use-recursive-partitioning)";
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-all-ranks,use-recursive-partitioning)";
    case StrategyStep::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,dont-use-local-rank,use-recursive-partitioning)";
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-local-rank,use-recursive-partitioning)";
  }
  return "<undef>";
}


toolbox::loadbalancing::RecursiveSubdivision::StrategyStep toolbox::loadbalancing::RecursiveSubdivision::getStrategyStep() const {
  if ( not _enabled ) {
    return StrategyStep::Wait;
  }

  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > peano4::parallel::Node::MaxSpacetreesPerRank/4*3
  ) {
    logDebug( "getStrategyStep()", "afraid to use too many trees and hence to overbook system" );
    return StrategyStep::Wait;
  }

  if (
    _state==StrategyState::PostponedDecisionDueToLackOfCells
    or
    _state==StrategyState::Stagnation
  ) {
    return StrategyStep::Wait;
  }

  if (
    not _hasSpreadOutOverAllRanks
    and
    tarch::mpi::Rank::getInstance().getNumberOfRanks()>1
    and
    tarch::mpi::Rank::getInstance().isGlobalMaster()
  ) {
    return StrategyStep::SpreadEquallyOverAllRanks;
  }

  const bool rankViolatesBalancingCondition = doesRankViolateBalancingCondition();

  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()==1
    and
    tarch::multicore::Core::getInstance().getNumberOfThreads()>1
    and
    canSplitLocally()
  ) {
    // @todo Debug
    logInfo(
      "getStrategyStep()",
      "local rank does only employ one thread, so try to spread out locally first"
    );
    return StrategyStep::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning;
  }


  // solely for info
  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    rankViolatesBalancingCondition
    and
    _state == StrategyState::WaitForRoundRobinToken
  ) {
    logInfo(
      "getStrategyStep()",
      "local rank violates global balancing condition, but I'm waiting for round robin token: " << toString()
    );
  }
  else if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    rankViolatesBalancingCondition
    and
    not canSplitLocally()
  ) {
    logInfo(
      "getStrategyStep()",
      "local rank violates global balancing condition, but is not allowed to split"
    );
  }

  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    rankViolatesBalancingCondition
    and
    _state != StrategyState::WaitForRoundRobinToken
    and
    canSplitLocally()
  ) {
    return peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>=tarch::multicore::Core::getInstance().getNumberOfThreads()
         ? StrategyStep::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning
         : StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning;
  }

  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() < tarch::multicore::Core::getInstance().getNumberOfThreads()
    and
    not rankViolatesBalancingCondition
    //and
    //_state==StrategyState::WaitForRoundRobinToken
    and
    canSplitLocally()
  ) {
    logInfo(
      "getStrategyStep()",
      "rank does not violate global balancing condition and should not balance globally anyway, so try to exploit more local cores"
    );
    return StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning;
  }

  logDebug( "getStrategyStep()", "no if becomes valid: " << rankViolatesBalancingCondition << " X " << canSplitLocally() << " X " << toString() );

  return StrategyStep::Wait;
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString( StrategyState state ) {
  switch (state) {
    case StrategyState::Standard:
      return "standard";
    case StrategyState::WaitForRoundRobinToken:
      return "wait-for-round-robin-token";
    case StrategyState::PostponedDecisionDueToLackOfCells:
      return "postponed-due-to-lack-of-cells";
    case StrategyState::Stagnation:
      return "stagnation";
  }
  return "<undef>";
}


bool toolbox::loadbalancing::RecursiveSubdivision::canSplitLocally() const {
  // This is a magic paramter with the 2. You might still run out of memory if you refine while you
  // rebalance.
  int worstCaseEstimateForSizeOfSpacetree = 2 * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
  return tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) >= worstCaseEstimateForSizeOfSpacetree;
}


int toolbox::loadbalancing::RecursiveSubdivision::getNumberOfSplitsOnLocalRank() const {
  int numberOfSplits     = std::max(1,tarch::multicore::Core::getInstance().getNumberOfThreads()-1);
  int maxSizeOfLocalRank = getWeightOfHeaviestLocalSpacetree();

  // The @f$ 3^d @f$ is to be sure that we can accommodate a regular refinement pattern 
  int worstCaseEstimateForSizeOfSpacetree = ThreePowerD * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
  int maxAdditionalSplitsDueToMemory      = tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) / worstCaseEstimateForSizeOfSpacetree;

  if (maxAdditionalSplitsDueToMemory==0) {
    logInfo( 
      "getNumberOfSplitsOnLocalRank(...)", 
       "not sure if additional tree fits on node; in particular if mesh should refine regularly. Split once instead of " << numberOfSplits << 
       " times (current mem footprint=" << worstCaseEstimateForSizeOfSpacetree << " MBytes, free mem=" <<
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << " MBytes)" 
    );
    numberOfSplits = 1;
  }
  else if (maxAdditionalSplitsDueToMemory<numberOfSplits) {
    // int adoptedSplitCount = maxAdditionalSplitsDueToMemory + 1;
    int adoptedSplitCount = std::min( maxAdditionalSplitsDueToMemory*2, numberOfSplits/2+1 );
    logInfo( 
      "getNumberOfSplitsOnLocalRank(...)", 
       "not sure if additional trees fit on node. Optimal number of splits is " << numberOfSplits << 
       ". With current mem footprint of " << worstCaseEstimateForSizeOfSpacetree << " MByte and free memory of " <<
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << ", we manually reduce split count to " << adoptedSplitCount
    );
    numberOfSplits = adoptedSplitCount; 
  }
  else {
    logInfo(
      "getNumberOfSplitsOnLocalRank(...)",
       "there seems to be enough memory available, so split " << numberOfSplits <<
       " times (current mem footprint=" << worstCaseEstimateForSizeOfSpacetree << " MByte, free memory=" <<
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << " MByte)"
    );
  }

  numberOfSplits  = std::max( numberOfSplits, 1 );
  return numberOfSplits;
}


void toolbox::loadbalancing::RecursiveSubdivision::finishStep() {
  updateGlobalView();
  updateBlacklist();
  updateState();

  auto step = getStrategyStep();

  #if PeanoDebug>0
  #else
  if ( step!=StrategyStep::Wait ) 
  #endif
  logInfo( "finishStep()", toString( step ) << " in state " << toString() );

  switch ( step ) {
    case StrategyStep::Wait:
      break;
    case StrategyStep::SpreadEquallyOverAllRanks:
      {
        int cellsPerRank = 
          std::max( static_cast<int>(std::round(_globalNumberOfInnerUnrefinedCells / tarch::mpi::Rank::getInstance().getNumberOfRanks())), 1);

        _hasSpreadOutOverAllRanks = true;

        for (int targetRank=1; targetRank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); targetRank++ ) {
          int thisRanksCells = cellsPerRank;
          if (static_cast<int>(_globalNumberOfInnerUnrefinedCells) % tarch::mpi::Rank::getInstance().getNumberOfRanks() >= targetRank) {
            thisRanksCells++;
          }
          triggerSplit(0, thisRanksCells, targetRank);
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _blacklist.count(heaviestSpacetree)==0) {
       	  int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          int cellsPerCore      = std::max(1,numberOfLocalUnrefinedCellsOfHeaviestSpacetree/tarch::multicore::Core::getInstance().getNumberOfThreads());
          int numberOfSplits    = getNumberOfSplitsOnLocalRank();

          logInfo( "finishStep()", "insufficient number of cores occupied on this rank, so split " << cellsPerCore << " cells " << numberOfSplits << " times from tree " << heaviestSpacetree << " on local rank (hosts " << numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " unrefined cells)" );

          for (int i=0; i<numberOfSplits; i++) {
            triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
          }
        }
        else {
          logInfo( "finishStep()", "local tree is not yet available" );
        } 
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _blacklist.count(heaviestSpacetree)==0) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,1);
          logInfo(
            "finishStep()",
            "lightest global rank is rank " << _lightestRank._rank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank._rank);
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _blacklist.count(heaviestSpacetree)==0 and _lightestRank._rank!=tarch::mpi::Rank::getInstance().getRank()) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,1);
          logInfo(
            "finishStep()",
            "lightest global rank is rank " << _lightestRank._rank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank._rank);
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _blacklist.count(heaviestSpacetree)==0) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          int cellsPerCore      = std::max(numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,1);
          logInfo(
            "finishStep()",
            "split local rank and assign it " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
        }
      }
      break;
  }

  dumpStatistics();
}



void toolbox::loadbalancing::RecursiveSubdivision::updateBlacklist() {
  for (std::map<int,int>::iterator p = _blacklist.begin(); p!=_blacklist.end(); ) {
    if (p->second>0) {
      p->second--;
      p++;
    }
    else {
      p = _blacklist.erase(p);
    }
  }
}


void toolbox::loadbalancing::RecursiveSubdivision::triggerSplit( int sourceTree, int numberOfCells, int targetRank ) {
  assertion(_enabled); 

  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,numberOfCells,targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  if ( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()<=1 ) {
    logInfo( "triggerSplit()", "do not insert any data into blacklist, as this seems to be the first split on the rank" );
  }
  else if ( _blacklist.count(sourceTree)==0 ) {
    const int InitialBlacklistWeight = 3;
    _blacklist.insert( std::pair<int,int>(sourceTree,InitialBlacklistWeight) );
  }
  else {
    _blacklist[sourceTree]++;
  }

  // Not always known a priori for example when we spread accross all
  // local ranks, then this field might not be yet set.
  if (getWeightOfHeaviestLocalSpacetree()>0) {
    _maxTreeWeightAtLastSplit = getWeightOfHeaviestLocalSpacetree();
  }

  _localNumberOfSplits++;
}


bool toolbox::loadbalancing::RecursiveSubdivision::hasSplitRecently() const {
  return _numberOfStateUpdatesWithoutAnySplit<3;
}


void toolbox::loadbalancing::RecursiveSubdivision::enable( bool value ) {
  _enabled = value;
}


bool toolbox::loadbalancing::RecursiveSubdivision::isEnabled(bool globally) const {
  logInfo( "isEnabled(bool)", toString() );
  return globally ? (_globalNumberOfRanksWithEnabledLoadBalancing>0) : _enabled;
}


int toolbox::loadbalancing::RecursiveSubdivision::getGlobalNumberOfTrees() const {
  return _globalNumberOfTrees;
}

