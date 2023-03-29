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


toolbox::loadbalancing::RecursiveSubdivision::RecursiveSubdivision(Configuration* configuration):
  _blacklist(),
  _initialBlacklistWeight(),
  _localNumberOfInnerUnrefinedCell( 0 ),
  _globalNumberOfInnerUnrefinedCells( 0 ),
  _globalNumberOfTrees(1),
  _globalNumberOfRanksWithEnabledLoadBalancing(0),
  _localNumberOfSplits(0),
  _numberOfStateUpdatesWithoutAnySplit(0),
  _stepsToWaitForNextLoadBalancingDecision(0),
  _state( State::InterRankDistribution ),
  _roundRobinToken(0),
  _configuration(configuration) {
  #ifdef Parallel
  _globalSumRequest            = nullptr;
  _globalLightestRankRequest   = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfTreesRequest  = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  #endif
  assertion(configuration!=nullptr);
}


toolbox::loadbalancing::RecursiveSubdivision::~RecursiveSubdivision() {
  delete _configuration;
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString() const {
  std::ostringstream msg;

  msg << "(state=" << toString( _state ) 
      << ",global-cell-count=" << _globalNumberOfInnerUnrefinedCells 
      << ",lightest-rank-no=" << _lightestRank._rank
      << ",lightest-rank-weight=" << _lightestRank._unrefinedCells
      << ",round-robin-token=" << _roundRobinToken 
      << ",number-of-state-updated-without-any-split=" << _numberOfStateUpdatesWithoutAnySplit
      << ",steps-to-wait-for-next-load-balancing-decision=" << _stepsToWaitForNextLoadBalancingDecision
      << ",global-number-of-splits=" << _globalNumberOfSplits
      << ",local-number-of-splits=" << _localNumberOfSplits
      << ",global-number-of-trees=" << _globalNumberOfTrees
      << ",global-number-of-ranks-with-enabled-lb=" << _globalNumberOfRanksWithEnabledLoadBalancing;

  if (_blacklist.empty()) {
    msg << ",blacklist is empty";
  }
  else {
    msg << ",blacklist={";
    for (auto p: _blacklist) {
      msg << "(#" << p.first << ":" << p.second
          << "/" << _initialBlacklistWeight.at(p.first)
          << ")";
    }
    msg << "}";
  }

  msg << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
      << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
      << ",#local-trees=" << peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() 
      << ",is-local-balancing-bad=" << isLocalBalancingBad()
      << ")";

  return msg.str();
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

  if (_stepsToWaitForNextLoadBalancingDecision>0) {
    _stepsToWaitForNextLoadBalancingDecision--;
  }

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCell;
    _lightestRank._rank                          = 1;
    _lightestRank._unrefinedCells                = _localNumberOfInnerUnrefinedCell;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
    _globalNumberOfRanksWithEnabledLoadBalancing = (_state!=State::SwitchedOff) ? 1 : 0;;
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

    if ( _globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCell ) {
      logInfo(
        "updateGlobalView()",
        "local number of cells (" << _localNumberOfInnerUnrefinedCell << ") is bigger than global cell count (" << _globalNumberOfInnerUnrefinedCells <<
        "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Postpone further load balancing decisions"
      );
      _stepsToWaitForNextLoadBalancingDecision = 3;
      _globalNumberOfInnerUnrefinedCells       = _localNumberOfInnerUnrefinedCell;
      _lightestRank._rank                      = tarch::mpi::Rank::getInstance().getRank();
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
    _numberOfRanksWithEnabledLoadBalancingOut       = isEnabled(false) ? 1 : 0;

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

  const double optimalCellCount = static_cast<double>(_globalNumberOfInnerUnrefinedCells)
                         / tarch::mpi::Rank::getInstance().getNumberOfRanks();

  const double illbalancing   = (static_cast<double>(localCells)-optimalCellCount) / optimalCellCount;
  const double balancingRatio = _configuration->getWorstCaseBalancingRatio(_state);
  bool result = (localCells < _globalNumberOfInnerUnrefinedCells) and (illbalancing > 1.0 - balancingRatio);

  if (result) {
    logInfo( "doesRankViolateBalancingCondition()", "rank does violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _globalNumberOfInnerUnrefinedCells << ", local cells=" << localCells << ", optimal cell count=" << optimalCellCount << ")" );
  }
  else {
    logDebug( "doesRankViolateBalancingCondition()", "rank does not violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _globalNumberOfInnerUnrefinedCells << ", local cells=" << localCells << ", optimal cell count=" << optimalCellCount << ")" );
  }

  return result;
}


void toolbox::loadbalancing::RecursiveSubdivision::updateState() {
  logDebug( "updateState()", "update from current state " << toString(_state) );
  switch (_state) {
    case State::InterRankDistribution:
      if (
        _globalNumberOfTrees > 1
        or
        tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1
      ) {
        _state = State::IntraRankDistribution;
        logDebug( "updateState()", "switch to state " << toString(_state) );
      }
      break;
    case State::IntraRankDistribution:
      if (
        peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 1
      ) {
        _state = State::WaitForRoundRobinToken;
        logDebug( "updateState()", "switch to state " << toString(_state) << " as more than one local subpartition is already hosted, i.e. we assume rank has already spread out locally" );
      }
      if (
        tarch::multicore::Core::getInstance().getNumberOfThreads()<=1
      ) {
        _state = State::WaitForRoundRobinToken;
        logDebug( "updateState()", "switch to state " << toString(_state) << " as rank is degenerated with only one thread, i.e. rank-local distribution makes no sense" );
      }
      break;
    case State::WaitForRoundRobinToken:
      {
        _roundRobinToken++;
        _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();

        if (_roundRobinToken==tarch::mpi::Rank::getInstance().getRank()) {
          _state = State::InterRankBalancing;
          logDebug( "updateState()", "switch to state " << toString(_state) );
        }
      }
      break;
    case State::InterRankBalancing:
      {
        if (
          tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1
          or
          !hasSplitRecently()
        ) {
          _state = State::IntraRankBalancing;
          logDebug( "updateState()", "switch to state " << toString(_state) );
        }
        else {
          _state = State::WaitForRoundRobinToken;
          logDebug( "updateState()", "switch to state " << toString(_state) );
        }
      }
      break;
    case State::IntraRankBalancing:
      {
        if (
          getWeightOfHeaviestLocalSpacetree()==-1
          or
          tarch::multicore::Core::getInstance().getNumberOfThreads()<=1
         ) {
          _state = State::Stagnation;
          logDebug( "updateState()", "switch to state " << toString(_state) );
        }
      }
      break;
    case State::Stagnation:
      if ( getWeightOfHeaviestLocalSpacetree()==-1) {
        _state = State::Stagnation;
        logDebug( "updateState()", "switch to state " << toString(_state) );
      }
      else {
        _state = State::WaitForRoundRobinToken;
        logDebug( "updateState()", "switch to state " << toString(_state) );
      }
      break;
    case State::SwitchedOff:
      break;
  }
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString( Action action ) {
  switch ( action ) {
    case Action::None:
      return "none";
    case Action::SpreadEquallyOverAllRanks:
      return "spread-equally-over-all-ranks";
    case Action::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(multiple-times,use-local-rank,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-all-ranks,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,dont-use-local-rank,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-local-rank,use-recursive-partitioning)";
    case Action::Unspecified:
      return "unspecified";
  }
  return "<undef>";
}


toolbox::loadbalancing::RecursiveSubdivision::Action toolbox::loadbalancing::RecursiveSubdivision::getAction() const {
  if (_stepsToWaitForNextLoadBalancingDecision>0) {
    return Action::None;
  }

  switch (_state) {
    case State::InterRankDistribution:
      {
        const int MinOriginalTreeSizeToTriggerMPISpreadOut = std::max(
          _configuration->getMinTreeSize(_state) * tarch::mpi::Rank::getInstance().getNumberOfRanks(),
          tarch::mpi::Rank::getInstance().getNumberOfRanks()
        );

        if (
          tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1
          or
          not tarch::mpi::Rank::getInstance().isGlobalMaster()
          or
          hasSplitRecently()
        ) {
          return Action::None;
        }
        else if ( _localNumberOfInnerUnrefinedCell < MinOriginalTreeSizeToTriggerMPISpreadOut ) {
          logInfo( "getAction()", "have to postpone any decision, as local no of inner unrefined cells of " << _localNumberOfInnerUnrefinedCell << " is smaller than " << MinOriginalTreeSizeToTriggerMPISpreadOut << " (which would occupy all ranks)" );
          return Action::None;
        }
        else {
          return Action::SpreadEquallyOverAllRanks;
        }
      }
      break;
    case State::IntraRankDistribution:
      {
        const int ThreadsToKeepBusy = std::min( tarch::multicore::Core::getInstance().getNumberOfThreads(), _configuration->getMaxLocalTreesPerRank(_state) );
        const int MinOriginalTreeSizeToTriggerThreadSpreadOut = std::max(
          _configuration->getMinTreeSize(_state) * ThreadsToKeepBusy,
          tarch::multicore::Core::getInstance().getNumberOfThreads()
        );

        if ( _localNumberOfInnerUnrefinedCell < MinOriginalTreeSizeToTriggerThreadSpreadOut ) {
          logInfo( "getAction()", "have to postpone any decision, as local no of inner unrefined cells of " << _localNumberOfInnerUnrefinedCell << " is smaller than " << MinOriginalTreeSizeToTriggerThreadSpreadOut << " (which would occupy all threads)" );
          return Action::None;
        }
        else {
          return Action::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning;
        }
      }
      break;
    case State::InterRankBalancing:
      {
        if (
          peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
          and
          doesRankViolateBalancingCondition()
          and
          getWeightOfHeaviestLocalSpacetree() < _configuration->getMinTreeSize(_state)
        ) {
          static bool wroteWarning = false;
          if (not wroteWarning) {
            logInfo( "getStrategyStep()", "local rank is not big enough, so we postpone re-balancing" );
            wroteWarning = true;
          }
        }
        else if (
          peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
          and
          doesRankViolateBalancingCondition()
        ) {
          if (
            fitsIntoMemory(_state)
            and
            peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() < _configuration->getMaxLocalTreesPerRank(_state)
          ) {
            return Action::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning;
          }
          else {
            return Action::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning;
          }
        }

        return Action::None;
      }
      break;
    case State::IntraRankBalancing:
      {
        // =================================
        // A posteriori intra-rank balancing
        // =================================
        // Need at least two local trees to be able to compare max and min
        if (
          peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
          and
          not doesRankViolateBalancingCondition()
          and
          isLocalBalancingBad()
        ) {
          if (
            peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() >= _configuration->getMaxLocalTreesPerRank(_state)
          ) {
            static bool wroteWarning = false;
            if (not wroteWarning) {
              logInfo(
                "getStrategyStep()",
                "local rank does not violate balancing condition between ranks but its local balancing is not good. However, there are already " <<
                peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() << " subpartitions locally and we are only supposed to use " <<
                _configuration->getMaxLocalTreesPerRank(_state) << ". Skip any further domain decomposition therefore"
              );
              wroteWarning = true;
            }
            return Action::None;
          }

          if (
            getWeightOfHeaviestLocalSpacetree() < _configuration->getMinTreeSize(_state)
          ) {
            static bool wroteWarning = false;
            if (not wroteWarning) {
              logInfo( "getStrategyStep()", "local rank is not big enough, so we postpone re-balancing even though partitions per rank are ill-balanced" );
              wroteWarning = true;
            }
            return Action::None;
          }

          return Action::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning;
        }
        return Action::None;
      }
      break;
    case State::WaitForRoundRobinToken:
    case State::Stagnation:
    case State::SwitchedOff:
      return Action::None;
  }

  assertion1(false, toString(_state));
  return Action::None;
}


bool toolbox::loadbalancing::RecursiveSubdivision::isLocalBalancingBad() const {
  int maxCells = 0;
  int minCells = std::numeric_limits<int>::max();

  for (auto p: peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees() ) {
    maxCells = std::max( maxCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() );
    minCells = std::min( minCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() );
  }

  bool result = false;

  if (maxCells>0 and minCells>0) {
    double relativeWeightSmalltestTree = static_cast<double>(minCells) / static_cast<double>(maxCells);
    double balanceThreshold            = _configuration->getWorstCaseBalancingRatio(_state);

    result = relativeWeightSmalltestTree < balanceThreshold;

    logDebug(
      "isLocalBalancingBad()",
      "local trees are ill-balanced=" << result <<
      ", rel-weight=" << relativeWeightSmalltestTree <<
      ", threshold=" << balanceThreshold <<
      ", #min-cells=" << minCells <<
      ", #max-cells=" << maxCells
    );
  }

  return result;
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString( State state ) {
  switch (state) {
    case State::InterRankDistribution:
      return "inter-rank-distribution";
    case State::IntraRankDistribution:
      return "intra-rank-distribution";
    case State::InterRankBalancing:
      return "inter-rank-balancing";
    case State::IntraRankBalancing:
      return "intra-rank-balancing";
    case State::WaitForRoundRobinToken:
      return "wait-for-round-robin";
    case State::Stagnation:
      return "stagnation";
    case State::SwitchedOff:
      return "switched-off";
  }
  return "<undef>";
}


bool toolbox::loadbalancing::RecursiveSubdivision::fitsIntoMemory(State state) const {
  // This is a magic paramter with the 2. You might still run out of memory if you refine while you
  // rebalance.
  if ( _configuration->makeSplitDependOnMemory(_state) ) {
    int worstCaseEstimateForSizeOfSpacetree = 2 * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
    return tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) >= worstCaseEstimateForSizeOfSpacetree;
  }
  else return true;
}


int toolbox::loadbalancing::RecursiveSubdivision::getNumberOfSplitsOnLocalRank() const {
  int maxSizeOfLocalRank = getWeightOfHeaviestLocalSpacetree();
  int maxLocalTrees      = std::min( {_configuration->getMaxLocalTreesPerRank(_state)-1, maxSizeOfLocalRank-1, tarch::multicore::Core::getInstance().getNumberOfThreads()-1} );
  int numberOfSplits     = std::max(1,maxLocalTrees);

  int worstCaseEstimateForSizeOfSpacetree = tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
  int maxAdditionalSplitsDueToMemory      = tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) / worstCaseEstimateForSizeOfSpacetree;
  int estimatedCellsPerTree               = maxSizeOfLocalRank / (numberOfSplits+1);

  const int MinTreeSize = _configuration->getMinTreeSize(_state);
  if ( estimatedCellsPerTree<MinTreeSize ) {
    const int adoptedSplits = std::max(1, maxSizeOfLocalRank / MinTreeSize - 1 );
    logInfo(
      "getNumberOfSplitsOnLocalRank(...)",
       "coded wanted to split " << numberOfSplits <<
       " times, but this would yield around " << estimatedCellsPerTree <<
	   " cells per tree, whereas the number of cells per tree should be at least " << MinTreeSize <<
	   ". Split only " << adoptedSplits << " times"
    );
    numberOfSplits = adoptedSplits;
  }
  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()<=1
    or
    maxAdditionalSplitsDueToMemory>=numberOfSplits
  ) {
    logInfo(
      "getNumberOfSplitsOnLocalRank(...)",
       "assume enough memory is available, so split " << numberOfSplits <<
       " times (current mem footprint=" << worstCaseEstimateForSizeOfSpacetree << " MByte, free memory=" <<
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << " MByte, est. cells per tree=" << estimatedCellsPerTree << 
       ", max-local-trees-per-rank=" << _configuration->getMaxLocalTreesPerRank(_state) << ", no-of-threads=" << 
       tarch::multicore::Core::getInstance().getNumberOfThreads() << ")"
    );
  }
  else if ( _configuration->makeSplitDependOnMemory(_state) ) {
    int adoptedSplitCount = std::max(1,maxAdditionalSplitsDueToMemory);
    logInfo( 
      "getNumberOfSplitsOnLocalRank(...)", 
       "not sure if additional trees fit on node. Optimal number of splits is " << numberOfSplits << 
       ". With current mem footprint of " << worstCaseEstimateForSizeOfSpacetree << " MByte and free memory of " <<
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << ", we manually reduce split count to " << adoptedSplitCount
    );
    numberOfSplits = adoptedSplitCount; 
  }

  return numberOfSplits;
}


void toolbox::loadbalancing::RecursiveSubdivision::updateLoadBalancing() {
  auto action = getAction();

  #if PeanoDebug>0
  logInfo( "updateLoadBalancing()", "load balancing's action " << toString(action) << " with internal state " << toString() );
  #else
  if ( action!=Action::None ) {
    logInfo( "updateLoadBalancing()", "load balancing's action " << toString(action) << " with internal state" << toString() );
  }
  #endif

  switch ( action ) {
    case Action::None:
      break;
    case Action::SpreadEquallyOverAllRanks:
      {
        int cellsPerRank = std::max( 
          static_cast<int>(std::round(_globalNumberOfInnerUnrefinedCells / tarch::mpi::Rank::getInstance().getNumberOfRanks())),
          1
        );

        for (int targetRank=1; targetRank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); targetRank++ ) {
          int thisRanksCells = cellsPerRank;
          if (static_cast<int>(_globalNumberOfInnerUnrefinedCells) % tarch::mpi::Rank::getInstance().getNumberOfRanks() >= targetRank) {
            thisRanksCells++;
          }
          triggerSplit(0, thisRanksCells, targetRank);
        }
      }
      break;
    case Action::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _blacklist.count(heaviestSpacetree)==0) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          // This operation takes care of the max tree count and size
          int numberOfSplits    = getNumberOfSplitsOnLocalRank();
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/(numberOfSplits+1),_configuration->getMinTreeSize(_state)} );

          logInfo(
            "updateLoadBalancing()",
            "split " << cellsPerCore << " or " << (cellsPerCore+1) << " cells " << numberOfSplits <<
            " times from tree " << heaviestSpacetree << " on local rank (hosts " << numberOfLocalUnrefinedCellsOfHeaviestSpacetree <<
            " unrefined cells with " << tarch::multicore::Core::getInstance().getNumberOfThreads() << " threads per rank)" );

          for (int i=0; i<numberOfSplits; i++) {
            int thisCellsPerCore = cellsPerCore;
            if (i<numberOfLocalUnrefinedCellsOfHeaviestSpacetree % (numberOfSplits+1)) {
              thisCellsPerCore++;
            }
            triggerSplit(heaviestSpacetree, thisCellsPerCore, tarch::mpi::Rank::getInstance().getRank());
          }
        }
        else {
          logInfo( "updateLoadBalancing()", "local tree is not yet available for further splits (heaviest-spacetree=" << heaviestSpacetree << ", is-on-blacklist=" << _blacklist.count(heaviestSpacetree) << ")" );
        } 
      }
      break;
    case Action::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
        ) {
          logInfo(
            "updateLoadBalancing()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMinTreeSize(_state)} );
 
          logInfo(
            "updateLoadBalancing()",
            "lightest global rank is rank " << _lightestRank._rank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank._rank);
        }
      }
      break;
    case Action::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
          and
          _lightestRank._rank!=tarch::mpi::Rank::getInstance().getRank()
        ) {
          logInfo(
            "updateLoadBalancing()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2, _configuration->getMinTreeSize(_state)} );
          logInfo(
            "updateLoadBalancing()",
            "lightest global rank is rank " << _lightestRank._rank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank._rank);
        }
      }
      break;
    case Action::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
        ) {
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2, _configuration->getMinTreeSize(_state)} );
          logInfo(
            "updateLoadBalancing()",
            "split tree on local rank and assign it " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
        }
        else if (_blacklist.count(heaviestSpacetree)!=0) {
          logInfo(
            "updateLoadBalancing()",
            "can't split heaviest tree " << heaviestSpacetree << " as this tree is on the blacklist"
          );
        }
      }
      break;
    case Action::Unspecified:
      assertion(false);
      break;
  }
}


void toolbox::loadbalancing::RecursiveSubdivision::finishStep() {
  updateGlobalView();
  updateBlacklist();
  updateLoadBalancing();
  updateState();
  dumpStatistics();
}


void toolbox::loadbalancing::RecursiveSubdivision::updateBlacklist() {
  for (std::map<int,int>::iterator p = _blacklist.begin(); p!=_blacklist.end(); ) {
    if ( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p->first).getRemovedEmptySubtree() ) {
      logInfo( "updateBlacklist()", "tree " << p->first << " has already been on local blacklist with weight " << p->second << ". Keep it longer on blacklist as it has degenerated child" );
      _initialBlacklistWeight[p->first] += tarch::mpi::Rank::getInstance().getNumberOfRanks();
      p->second++;
      p++;
    }
    else if (p->second>0) {
      p->second--;
      p++;
    }
    else {
      logInfo( "updateBlacklist()", "remove tree " << p->first << " from blacklist" );
      p = _blacklist.erase(p);
    }
  }
}


void toolbox::loadbalancing::RecursiveSubdivision::triggerSplit( int sourceTree, int numberOfCells, int targetRank ) {
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,numberOfCells,targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  if ( _blacklist.count(sourceTree)==0 ) {
    if (_initialBlacklistWeight.count(sourceTree)==0) {
      _initialBlacklistWeight.insert( std::pair<int,int>(sourceTree,3) );
    }
    else {
      _initialBlacklistWeight[sourceTree]++;
    }
    
    const int InitialBlacklistWeight = _initialBlacklistWeight[sourceTree];
    _blacklist.insert( std::pair<int,int>(sourceTree,InitialBlacklistWeight) );
  }
  else {
    assertion( _initialBlacklistWeight.count(sourceTree)>0 );
    //_blacklist[sourceTree]++;
    _initialBlacklistWeight[sourceTree]++;
    logDebug(
      "triggerSplit()",
      "split local rank " << sourceTree << " though it had been on the blacklist"
    );
  }

  _localNumberOfSplits++;

  assertionEquals(_stepsToWaitForNextLoadBalancingDecision,0);
  _stepsToWaitForNextLoadBalancingDecision = 3;
}


bool toolbox::loadbalancing::RecursiveSubdivision::hasSplitRecently() const {
  return _numberOfStateUpdatesWithoutAnySplit<3;
}


void toolbox::loadbalancing::RecursiveSubdivision::enable( bool value ) {
  if (value and _state==State::SwitchedOff) {
    _state = State::InterRankBalancing;
    logInfo( "enable()", "switched load balancing on (again) and therefore change to state " << toString(_state) );
  }
  else if (not value) {
    logInfo( "enable()", "switched load balancing off" );
    _state = State::SwitchedOff;
  }
}


bool toolbox::loadbalancing::RecursiveSubdivision::isEnabled(bool globally) const {
  logDebug( "isEnabled(bool)", toString() );
  return globally ? (_globalNumberOfRanksWithEnabledLoadBalancing>0) : (_state != State::SwitchedOff);
}


int toolbox::loadbalancing::RecursiveSubdivision::getGlobalNumberOfTrees() const {
  return _globalNumberOfTrees;
}


