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
  _configuration(configuration) {
  #ifdef Parallel
  _globalSumRequest            = nullptr;
  _globalLightestRankRequest   = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfTreesRequest  = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  #else
  _hasSpreadOutOverAllRanks = true;
  #endif
  assertion(configuration!=nullptr);
}


toolbox::loadbalancing::RecursiveSubdivision::~RecursiveSubdivision() {
  delete _configuration;
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
      << ",number-of-state-updated-without-any-split=" << _numberOfStateUpdatesWithoutAnySplit
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
      << ",enabled=" <<  _enabled
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

  if (
    not tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    not _hasSpreadOutOverAllRanks
  ) {
    _hasSpreadOutOverAllRanks = true;
  }

  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    not _hasSpreadOutOverAllRanks
    and
    tarch::mpi::Rank::getInstance().getNumberOfRanks()==1
  ) {
    _hasSpreadOutOverAllRanks = true;
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

    if ( _globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCell ) {
      logInfo(
        "updateGlobalView()",
        "local number of cells (" << _localNumberOfInnerUnrefinedCell << ") is bigger than global cell count (" << _globalNumberOfInnerUnrefinedCells << 
        ") . Use local cell count to guide partitioning as local data seems to lag behind"
      );
      _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCell;
      _lightestRank._rank                = tarch::mpi::Rank::getInstance().getRank();
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

  const double optimalCellCount = static_cast<double>(_globalNumberOfInnerUnrefinedCells)
                         / tarch::mpi::Rank::getInstance().getNumberOfRanks();

  const double illbalancing   = (static_cast<double>(localCells)-optimalCellCount) / optimalCellCount;
  const double balancingRatio = _configuration->getWorstCaseBalancingRatio(Configuration::Phase::InterRankBalancing);
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
  _roundRobinToken++;
  _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();

  const int MinOriginalTreeSizeToTriggerMPISpreadOut = std::max(
    _configuration->getMinTreeSize(Configuration::Phase::InitialInterRankDistribution) * tarch::mpi::Rank::getInstance().getNumberOfRanks(),
    tarch::mpi::Rank::getInstance().getNumberOfRanks()
  );
  const int MinOriginalTreeSizeToTriggerThreadSpreadOut = std::max(
    _configuration->getMinTreeSize(Configuration::Phase::InitialIntraRankDistribution) * tarch::multicore::Core::getInstance().getNumberOfThreads(),
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );

  if (
    not _hasSpreadOutOverAllRanks
    and
    _localNumberOfInnerUnrefinedCell < MinOriginalTreeSizeToTriggerMPISpreadOut
  ) {
    _state = StrategyState::PostponedDecisionDueToLackOfCells;
  }
  else if (
    _hasSpreadOutOverAllRanks
    and
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()==1
    and
    _localNumberOfInnerUnrefinedCell < MinOriginalTreeSizeToTriggerThreadSpreadOut
  ) {
    logInfo( "updateState()", "have to postpone any decision, as local no of inner unrefined cells of " << _localNumberOfInnerUnrefinedCell << " is smaller than " << MinOriginalTreeSizeToTriggerThreadSpreadOut << " (need to give each thread at least around " << _configuration->getMinTreeSize(Configuration::Phase::InitialIntraRankDistribution) << " cells)" );
    _state = StrategyState::PostponedDecisionDueToLackOfCells;
  }
  else if (
    _hasSpreadOutOverAllRanks
    and
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
    and
    _localNumberOfInnerUnrefinedCell < _configuration->getMinTreeSize(Configuration::Phase::InterRankBalancing)
  ) {
    _state = StrategyState::PostponedDecisionDueToLackOfCells;
  }
  else if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
    and
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
    case StrategyStep::Unspecified:
      return "unspecified";
  }
  return "<undef>";
}


toolbox::loadbalancing::RecursiveSubdivision::StrategyStep toolbox::loadbalancing::RecursiveSubdivision::getStrategyStep() const {
  if ( not _enabled ) {
    return StrategyStep::Wait;
  }


  if (
    _state==StrategyState::PostponedDecisionDueToLackOfCells
    or
    _state==StrategyState::Stagnation
  ) {
    return StrategyStep::Wait;
  }


  // =========================
  // Spread out over all ranks
  // =========================
  if (
    not _hasSpreadOutOverAllRanks
    and
    tarch::mpi::Rank::getInstance().getNumberOfRanks()>1
    and
    tarch::mpi::Rank::getInstance().isGlobalMaster()
  ) {
    logInfo(
      "getStrategyStep()",
      "spread out over all ranks"
    );
    return StrategyStep::SpreadEquallyOverAllRanks;
  }

  // ===============================
  // Spread out over the local cores
  // -------------------------------
  // This is the one place where we don't have to analyse memory and tree count,
  // as this will be done later down the line.
  // ===============================
  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()==1
    and
    tarch::multicore::Core::getInstance().getNumberOfThreads()>1
  ) {
    logInfo(
      "getStrategyStep()",
      "local rank does only employ one thread, so try to spread out locally first"
    );
    if ( fitsIntoMemory(Configuration::Phase::InitialIntraRankDistribution) ) {
      return StrategyStep::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning;
    }
    else {
      return StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning;
    }
  }

  // =========================
  // Inter-rank load balancing
  // =========================
  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
    and
    doesRankViolateBalancingCondition()
  ) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 2*std::min(tarch::multicore::Core::getInstance().getNumberOfThreads(),_configuration->getMaxLocalTreesPerRank(Configuration::Phase::InterRankBalancing))
    ) {
      static bool wroteWarning = false;
      if (not wroteWarning) {
        logInfo( "getStrategyStep()", "local rank violates balancing condition and thus should split up further. However, there are already so many subpartitions locally that we assume that we already have a quite fine-granular decomposition. Skip further split-ups therefore" );
        wroteWarning = true;
      }
      return StrategyStep::Wait;
    }

    if (
      getWeightOfHeaviestLocalSpacetree() < _configuration->getMinTreeSize(Configuration::Phase::InterRankBalancing)
    ) {
      static bool wroteWarning = false;
      if (not wroteWarning) {
        logInfo( "getStrategyStep()", "local rank is not big enough, so we postpone re-balancing" );
        wroteWarning = true;
      }
      return StrategyStep::Wait;
    }

    if (
      _state == StrategyState::WaitForRoundRobinToken
    ) {
      logInfo(
        "getStrategyStep()",
        "local rank violates global balancing condition, but I'm waiting for round robin token: " << toString()
      );
      return StrategyStep::Wait;
    }

    logInfo(
      "getStrategyStep()",
      "local rank violates balancing condition and thus has to split up"
    );

    if ( fitsIntoMemory(Configuration::Phase::InterRankBalancing) ) {
      return StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning;
    }
    else {
      return StrategyStep::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning;
    }
  }

  // =================================
  // A posteriori intra-rank balancing
  // =================================
  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1
    and
    not doesRankViolateBalancingCondition()
    and
    isLocalBalancingBad()
  ) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() >
      std::min( tarch::multicore::Core::getInstance().getNumberOfThreads()-1, _configuration->getMaxLocalTreesPerRank(Configuration::Phase::APosterioriBalancingPerRank))
    ) {
      static bool wroteWarning = false;
      if (not wroteWarning) {
        logInfo( "getStrategyStep()", "local rank does not violate balancing condition between ranks but its local balancing is not good. However, there are already so many subpartitions locally that we assume that we already have a quite fine-granular decomposition. Skip further split-ups therefore" );
        wroteWarning = true;
      }
      return StrategyStep::Wait;
    }

    if (
      getWeightOfHeaviestLocalSpacetree() < _configuration->getMinTreeSize(Configuration::Phase::APosterioriBalancingPerRank)
    ) {
      static bool wroteWarning = false;
      if (not wroteWarning) {
        logInfo( "getStrategyStep()", "local rank is not big enough, so we postpone re-balancing even though partitions per rank are ill-balanced" );
        wroteWarning = true;
      }
      return StrategyStep::Wait;
    }

    logInfo(
      "getStrategyStep()",
      "rank does not violate global balancing condition and should not balance globally anyway, but rank-local balancing is bad. Try to exploit more local cores"
    );
    return StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning;
  }

  logDebug(
    "getStrategyStep()",
    "no rule applies, so wait (local balancing is bad=" << isLocalBalancingBad() << ")"
  );

  return StrategyStep::Wait;
}


bool toolbox::loadbalancing::RecursiveSubdivision::isLocalBalancingBad() const {
  int maxCells = 0;
  int minCells = std::numeric_limits<int>::max();

  for (auto p: peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees() ) {
    maxCells = std::max( maxCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() );
    if ( 
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()
      >
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells() 
      +
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteRefinedCells()
    ) {
      minCells = std::min( minCells, peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() );
    }
  }

  bool result = minCells < maxCells
            and ( 1.0*minCells/maxCells ) < _configuration->getWorstCaseBalancingRatio(Configuration::Phase::APosterioriBalancingPerRank);

  return result;
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


bool toolbox::loadbalancing::RecursiveSubdivision::fitsIntoMemory(Configuration::Phase phase) const {
  // This is a magic paramter with the 2. You might still run out of memory if you refine while you
  // rebalance.
  if ( _configuration->makeSplitDependOnMemory(phase) ) {
    int worstCaseEstimateForSizeOfSpacetree = 2 * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
    return tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) >= worstCaseEstimateForSizeOfSpacetree;
  }
  else return true;
}


int toolbox::loadbalancing::RecursiveSubdivision::getNumberOfSplitsOnLocalRank() const {
  const Configuration::Phase phase = Configuration::Phase::InitialIntraRankDistribution;

  int numberOfSplits     = std::max(1, std::min(_configuration->getMaxLocalTreesPerRank(phase),tarch::multicore::Core::getInstance().getNumberOfThreads())-1);
  int maxSizeOfLocalRank = getWeightOfHeaviestLocalSpacetree();

  int worstCaseEstimateForSizeOfSpacetree = tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
  int maxAdditionalSplitsDueToMemory      = tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) / worstCaseEstimateForSizeOfSpacetree;
  int estimatedCellsPerTree               = maxSizeOfLocalRank / (numberOfSplits+1);

  const int MinTreeSize = _configuration->getMinTreeSize(phase);
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
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << " MByte, est. cells per tree=" << estimatedCellsPerTree << ")"
    );
  }
  else if ( _configuration->makeSplitDependOnMemory(phase) ) {
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


void toolbox::loadbalancing::RecursiveSubdivision::finishStep() {
  updateGlobalView();
  updateBlacklist();
  updateState();

  auto step = getStrategyStep();

//  #if PeanoDebug>0
//  #else
//  if ( step!=StrategyStep::Wait ) 
//  #endif
  logInfo( "finishStep()", toString( step ) << " in state " << toString() );

  switch ( step ) {
    case StrategyStep::Wait:
      break;
    case StrategyStep::SpreadEquallyOverAllRanks:
      {
        int cellsPerRank = std::max( 
          std::min( static_cast<int>(std::round(_globalNumberOfInnerUnrefinedCells / tarch::mpi::Rank::getInstance().getNumberOfRanks())), _configuration->getMaxTreeSize(Configuration::Phase::InitialInterRankDistribution) ),
          1);

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
       	  // This operation takes care of the max tree count and size
          int numberOfSplits    = getNumberOfSplitsOnLocalRank();
          int cellsPerCore      = std::max(1, std::min( numberOfLocalUnrefinedCellsOfHeaviestSpacetree/(numberOfSplits+1),_configuration->getMaxTreeSize(Configuration::Phase::InitialIntraRankDistribution) ));

          logInfo(
            "finishStep()",
            "split " << cellsPerCore << " cells " << numberOfSplits <<
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
          logInfo( "finishStep()", "local tree is not yet available for further splits (heaviest-spacetree=" << heaviestSpacetree << ", is-on-blacklist=" << _blacklist.count(heaviestSpacetree) << ")" );
        } 
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
        ) {
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(1, std::min( numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMaxTreeSize(Configuration::Phase::InterRankBalancing) ));
 
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
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
          and
          _lightestRank._rank!=tarch::mpi::Rank::getInstance().getRank()
        ) {
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(1, std::min( numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMaxTreeSize(Configuration::Phase::InterRankBalancing) ));
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
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          _blacklist.count(heaviestSpacetree)==0
        ) {
          int cellsPerCore      = std::max(1, std::min( numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMaxTreeSize(Configuration::Phase::InterRankBalancing) ));
          logInfo(
            "finishStep()",
            "split local rank and assign it " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
        }
        else if (_blacklist.count(heaviestSpacetree)!=0) {
          logInfo(
            "finishStep()",
            "can't split heaviest tree " << heaviestSpacetree << " as this rank is on the blacklist"
          );
        }
      }
      break;
    case StrategyStep::Unspecified:
      assertion(false);
      break;
  }

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
  assertion(_enabled); 

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
}


bool toolbox::loadbalancing::RecursiveSubdivision::hasSplitRecently() const {
  return _numberOfStateUpdatesWithoutAnySplit<3;
}


void toolbox::loadbalancing::RecursiveSubdivision::enable( bool value ) {
  _enabled = value;
}


bool toolbox::loadbalancing::RecursiveSubdivision::isEnabled(bool globally) const {
  logDebug( "isEnabled(bool)", toString() );
  return globally ? (_globalNumberOfRanksWithEnabledLoadBalancing>0) : _enabled;
}


int toolbox::loadbalancing::RecursiveSubdivision::getGlobalNumberOfTrees() const {
  return _globalNumberOfTrees;
}

