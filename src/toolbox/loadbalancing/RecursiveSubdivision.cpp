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
  _hasSpreadOutOverAllCores( false ),
  _localNumberOfInnerUnrefinedCell( 0 ),
  _globalNumberOfInnerUnrefinedCells( 0 ),
  _lightestRank(-1),
  _localNumberOfSplits(0),
  _localNumberOfSplitsPreviousStep(0),
  _globalNumberOfSplits(0),
  _state( StrategyState::Standard ),
  _roundRobinToken(0),
  _maxTreeWeightAtLastSplit( std::numeric_limits<int>::max() ),
  _blacklistWeight(MinBlacklistWeight)  {
  #ifdef Parallel
  _globalSumRequest            = nullptr;
  _globalLightestRankRequest   = nullptr;
  _globalNumberOfSplitsRequest = nullptr;
  _globalNumberOfInnerUnrefinedCellsBufferIn = 1;
  _lightestRankBufferIn._rank                = tarch::mpi::Rank::getInstance().getRank();
  _globalNumberOfSplitsIn                    = 0;
  #endif
}


std::string toolbox::loadbalancing::RecursiveSubdivision::toString() const {
  std::ostringstream msg;

  msg << "(state=" << toString( _state ) 
      << ",global-cell-count=" << _globalNumberOfInnerUnrefinedCells 
      << ",lightest-rank=" << _lightestRank
      << ",has-spread-over-all-ranks=" << _hasSpreadOutOverAllRanks 
      << ",has-spread-over-all-cores=" << _hasSpreadOutOverAllCores
      << ",round-robin-token=" << _roundRobinToken 
      << ",target-balancing-ratio=" << _TargetBalancingRatio
      << ",max-tree-weight-at-last-split=" << _maxTreeWeightAtLastSplit
      << ",blacklist-weight=" << _blacklistWeight; 

  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  for (auto p: idsOfLocalSpacetrees) {
    msg << ",tree " << p << ":" << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()
	  << ( _blacklist.count(p)>0 ? " (on blacklist with weight=" + std::to_string(_blacklist.at(p)) + ")" : "" );
  }

  if (_blacklist.empty()) {
    msg << ",blacklist is empty";
  }

  msg << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
      << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
      << ")";

  return msg.str();
}


toolbox::loadbalancing::RecursiveSubdivision::~RecursiveSubdivision() {
}


void toolbox::loadbalancing::RecursiveSubdivision::finishSimulation() {
  #ifdef Parallel
  if (_globalSumRequest != nullptr ) {
    MPI_Wait( _globalSumRequest,            MPI_STATUS_IGNORE );
    MPI_Wait( _globalLightestRankRequest,   MPI_STATUS_IGNORE );
    MPI_Wait( _globalNumberOfSplitsRequest, MPI_STATUS_IGNORE );
    _globalSumRequest            = nullptr;
    _globalLightestRankRequest   = nullptr;
    _globalNumberOfSplitsRequest = nullptr;
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

  _globalNumberOfSplits = _localNumberOfSplits + _localNumberOfSplitsPreviousStep;
  _localNumberOfSplitsPreviousStep = _localNumberOfSplits;
  _localNumberOfSplits  = 0;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCell;
    _lightestRank                      = 0;
  }
  else {
    #ifdef Parallel
    if (_globalSumRequest != nullptr ) {
      MPI_Wait( _globalSumRequest, MPI_STATUS_IGNORE );
      MPI_Wait( _globalLightestRankRequest, MPI_STATUS_IGNORE );
    }

    // rollover
    _globalNumberOfInnerUnrefinedCells  = static_cast<int>( std::round(_globalNumberOfInnerUnrefinedCellsBufferIn) );
    _lightestRank                       = _lightestRankBufferIn._rank;
    _globalNumberOfSplits              += _globalNumberOfSplitsIn;
    _localNumberOfSplits               += _globalNumberOfSplitsIn;

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
      _lightestRank                     = tarch::mpi::Rank::getInstance().getRank();
    }
    else if (_globalNumberOfInnerUnrefinedCells <= _localNumberOfInnerUnrefinedCell ) {
      _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCell * tarch::mpi::Rank::getInstance().getNumberOfRanks();
      logInfo(
        "updateGlobalView()",
        "global number of cells lags behind local one. Code has spread over ranks already. Therefore, assume that we use global data from previous time step. Extrapolate " << _localNumberOfInnerUnrefinedCell 
        << " to guide partitioning. Assume it equals " << _globalNumberOfInnerUnrefinedCells
      );
      _lightestRank                     = tarch::mpi::Rank::getInstance().getRank();
    }


    _globalSumRequest            = new MPI_Request();
    _globalLightestRankRequest   = new MPI_Request();
    _globalNumberOfSplitsRequest = new MPI_Request();

    _lightestRankBufferOut._localUnrefinedCells = _localNumberOfInnerUnrefinedCell;
    _lightestRankBufferOut._rank                = tarch::mpi::Rank::getInstance().getRank();

    MPI_Iallreduce(
      &_localNumberOfInnerUnrefinedCell,             // send
      &_globalNumberOfInnerUnrefinedCellsBufferIn,   // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalSumRequest
    );
    MPI_Iallreduce(
      &_lightestRankBufferOut,   // send
      &_lightestRankBufferIn,    // receive
      1,                         // count
      MPI_DOUBLE_INT,
      MPI_MINLOC,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalLightestRankRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfSplits,     // send
      &_globalNumberOfSplitsIn,   // receive
      1,             // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfSplitsRequest
    );
    #endif
  }
}


int toolbox::loadbalancing::RecursiveSubdivision::getIdOfHeaviestLocalSpacetree() const {
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  int result = NoHeaviestTreeAvailable;
  int maxLocalUnrefinedCells = -1;
  for (auto p: idsOfLocalSpacetrees) {
    if (
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells()>maxLocalUnrefinedCells
      and
      _blacklist.count(p)==0
    ) {
      maxLocalUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
      result = p;
    }
  }
  return result;
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
    logInfo( "doesRankViolateBalancingCondition()", "rank does violate balancing as we have ill-balancing of " << illbalancing << " (local cells=" << localCells << ", threshold=" << threshold << ", balancing-ratio=" << _TargetBalancingRatio << ")" );
  }
  else {
    logDebug( "doesRankViolateBalancingCondition()", "rank does not violate balancing as we have ill-balancing of " << illbalancing << " (local cells=" << localCells << ", threshold=" << threshold << ", balancing-ratio=" << _TargetBalancingRatio << ")" );
  }


  return result;
}


void toolbox::loadbalancing::RecursiveSubdivision::updateState() {
  _roundRobinToken++;
  _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();

  if ( _maxTreeWeightAtLastSplit == getWeightOfHeaviestLocalSpacetree() and _blacklistWeight < 65536) {
    _blacklistWeight *= 2;
  }
  else if ( _blacklistWeight>MinBlacklistWeight ) {
    _blacklistWeight--;
  }
    
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
  else if (_roundRobinToken!=tarch::mpi::Rank::getInstance().getRank()) {
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


  if (not canSplitLocally() and doesRankViolateBalancingCondition()) {
    logInfo( 
      "getStrategyStep()", 
      "cannot load balance further as memory available is too small" <<
      ". used-mem=" << tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte ) <<
      ", free-mem=" << tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) 
    );
  }


  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    static_cast<double>(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()) < tarch::multicore::Core::getInstance().getNumberOfThreads()
    and
    not _hasSpreadOutOverAllCores
    and
    canSplitLocally()
  ) {
    return StrategyStep::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning;
  }


  if (
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 0
    and
    doesRankViolateBalancingCondition()
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
    not doesRankViolateBalancingCondition()
    and
    _state != StrategyState::WaitForRoundRobinToken
    and
    peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() < tarch::multicore::Core::getInstance().getNumberOfThreads()
    and
    canSplitLocally()
  ) {
    // @todo debug
    logInfo( "getStrategyStep()", "rank is well-balanced, but does not yet use all local cores; split up rank's trees further" );
    return StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning;
  }

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


int toolbox::loadbalancing::RecursiveSubdivision::getWeightOfHeaviestLocalSpacetree() const {
  const int heaviestSpacetree = getIdOfHeaviestLocalSpacetree();
  return heaviestSpacetree==NoHeaviestTreeAvailable ? -1 : peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(heaviestSpacetree).getNumberOfLocalUnrefinedCells();
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
       ", With current mem footprint of " << worstCaseEstimateForSizeOfSpacetree << " MByte and free memory of " << 
       tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) << " we manually reduce split count to " << adoptedSplitCount
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
        int cellsPerRank = std::max( static_cast<int>(std::round(_globalNumberOfInnerUnrefinedCells / tarch::mpi::Rank::getInstance().getNumberOfRanks())), 1);

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
        if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
       	  int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          int cellsPerCore      = std::max(1,numberOfLocalUnrefinedCellsOfHeaviestSpacetree/tarch::multicore::Core::getInstance().getNumberOfThreads());
          int numberOfSplits    = getNumberOfSplitsOnLocalRank();

          logInfo( "getNumberOfSplitsOnLocalRank()", "insufficient number of cores occupied on this rank, so split " << cellsPerCore << " cells " << numberOfSplits << " times from tree " << heaviestSpacetree << " on local rank (hosts " << numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " unrefined cells)" );

          for (int i=0; i<numberOfSplits; i++) {
            triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
          }

          _hasSpreadOutOverAllCores = true;
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,1);
          logInfo(
            "finishStep()",
            "lightest global rank is rank " << _lightestRank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank);
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and _lightestRank!=tarch::mpi::Rank::getInstance().getRank()) {
          int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
          logInfo(
            "finishStep()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max(numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,1);
          logInfo(
            "finishStep()",
            "lightest global rank is rank " << _lightestRank << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _lightestRank);
        }
      }
      break;
    case StrategyStep::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
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
  if (not peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,numberOfCells,targetRank)) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }
  else {
    _localNumberOfSplits++;
  }


  _blacklist.insert( std::pair<int,int>(sourceTree,_blacklistWeight) );
  // Not always known a priori for example when we spread accross all
  // local ranks, then this field might not be yet set.
  if (getWeightOfHeaviestLocalSpacetree()>0) {
    _maxTreeWeightAtLastSplit = getWeightOfHeaviestLocalSpacetree();
  }
}


bool toolbox::loadbalancing::RecursiveSubdivision::hasSplitRecently() const {
  return _localNumberOfSplits + _globalNumberOfSplits>0;
}
