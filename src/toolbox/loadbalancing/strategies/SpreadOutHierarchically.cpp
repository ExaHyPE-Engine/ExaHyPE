#include "../strategies/SpreadOutHierarchically.h"

#include "toolbox/loadbalancing/loadbalancing.h"


#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log  toolbox::loadbalancing::strategies::SpreadOutHierarchically::_log( "toolbox::loadbalancing::strategies::SpreadOutHierarchically" );


std::string toolbox::loadbalancing::strategies::SpreadOutHierarchically::toString( Action action ) {
  switch ( action ) {
    case Action::Unspecified:
      return "unspecified";
    case Action::None:
      return "none";
    case Action::SpreadEquallyOverAllRanks:
      return "spread-equally-over-all-ranks";
    case Action::SpreadEquallyOverAllThreads:
      return "spread-equally-over-all-threads";
  }
  return "<undef>";
}


toolbox::loadbalancing::strategies::SpreadOutHierarchically::SpreadOutHierarchically(Configuration* configuration, CostMetrics*   costMetrics ):
  AbstractLoadBalancing( configuration, costMetrics),
  _stepsToWaitForNextLoadBalancingDecision(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::strategies::SpreadOutHierarchically::~SpreadOutHierarchically() {
}


void toolbox::loadbalancing::strategies::SpreadOutHierarchically::updateState() {
  switch (_state) {
    case State::InterRankDistribution:
      if (
        _statistics.getGlobalNumberOfTrees() > 1
        or
        tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1
      ) {
        _state = State::IntraRankDistribution;
        logDebug( "updateState()", "switch to state " << ::toString(_state) );
      }
      break;
    case State::IntraRankDistribution:
      if (
        peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() > 1
      ) {
        _state = State::Stagnation;
        logDebug( "updateState()", "switch to state " << ::toString(_state) << " as more than one local subpartition is already hosted, i.e. we assume rank has already spread out locally" );
      }
      if (
        tarch::multicore::Core::getInstance().getNumberOfThreads()<=1
      ) {
        _state = State::Stagnation;
        logDebug( "updateState()", "switch to state " << ::toString(_state) << " as rank is degenerated with only one thread, i.e. rank-local distribution makes no sense" );
      }
      break;
    default:
      break;
  }
}


toolbox::loadbalancing::strategies::SpreadOutHierarchically::Action toolbox::loadbalancing::strategies::SpreadOutHierarchically::getAction() const {
  if (_stepsToWaitForNextLoadBalancingDecision>0) {
    logDebug(
      "getAction()",
      "shall wait for another " << _stepsToWaitForNextLoadBalancingDecision << " step(s) before we rebalance"
    );
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
        else if ( _statistics.getLocalNumberOfInnerUnrefinedCells() < MinOriginalTreeSizeToTriggerMPISpreadOut ) {
          logInfo(
            "getAction()",
            "have to postpone any decision, as local no of inner unrefined cells of " << _statistics.getLocalNumberOfInnerUnrefinedCells() << " is smaller than " << MinOriginalTreeSizeToTriggerMPISpreadOut << " (which would occupy all ranks)" );
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

        if ( _statistics.getLocalNumberOfInnerUnrefinedCells() < MinOriginalTreeSizeToTriggerThreadSpreadOut ) {
          logInfo( "getAction()", "have to postpone any decision, as local no of inner unrefined cells of " << _statistics.getLocalNumberOfInnerUnrefinedCells() << " is smaller than " << MinOriginalTreeSizeToTriggerThreadSpreadOut << " (which would occupy all threads)" );
          return Action::None;
        }
        else {
          return Action::SpreadEquallyOverAllThreads;
        }
      }
      break;
    default:
      break;
  }

  return Action::None;
}


int toolbox::loadbalancing::strategies::SpreadOutHierarchically::getNumberOfSplitsOnLocalRank() const {
  assertionEquals( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1 );

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


void toolbox::loadbalancing::strategies::SpreadOutHierarchically::updateLoadBalancing() {
  auto action = getAction();

  #if PeanoDebug>0
  logInfo( "updateLoadBalancing()", "load balancing's action " << toString(action) << " with internal state " << AbstractLoadBalancing::toString() );
  #else
  if ( action!=Action::None ) {
    logInfo( "updateLoadBalancing()", "load balancing's action " << toString(action) << " with internal state" << AbstractLoadBalancing::toString() );
  }
  #endif

  switch ( action ) {
    case Action::SpreadEquallyOverAllRanks:
      {
        int cellsPerRank = std::max(
          static_cast<int>(std::round(_statistics.getGlobalNumberOfInnerUnrefinedCells() / tarch::mpi::Rank::getInstance().getNumberOfRanks())),
          1
        );

        for (int targetRank=1; targetRank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); targetRank++ ) {
          int thisRanksCells = cellsPerRank;
          if (static_cast<int>(_statistics.getGlobalNumberOfInnerUnrefinedCells()) % tarch::mpi::Rank::getInstance().getNumberOfRanks() >= targetRank) {
            thisRanksCells++;
          }
          triggerSplit(thisRanksCells, targetRank);
        }
      }
      break;
    case Action::SpreadEquallyOverAllThreads:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        if (heaviestSpacetree!=NoHeaviestTreeAvailable and not _blacklist.isBlacklisted(heaviestSpacetree) ) {
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
            triggerSplit(thisCellsPerCore, tarch::mpi::Rank::getInstance().getRank());
          }
        }
        else {
          logInfo( "updateLoadBalancing()", "local tree is not yet available for further splits (heaviest-spacetree=" << heaviestSpacetree << ")" );
        }
      }
      break;
    default:
      break;
  }
}


void toolbox::loadbalancing::strategies::SpreadOutHierarchically::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  if ( _statistics.hasConsistentViewOfWorld() ) {
    _stepsToWaitForNextLoadBalancingDecision  = std::max( _stepsToWaitForNextLoadBalancingDecision-1, 0 );
  }
  else {
   logInfo( "finishStep()", "statistics have no consistent view of world, so postpone load balancing decisions" );
    _stepsToWaitForNextLoadBalancingDecision = std::max(_stepsToWaitForNextLoadBalancingDecision,1);
  }

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange( _state );

  dumpStatistics();
}


void toolbox::loadbalancing::strategies::SpreadOutHierarchically::triggerSplit( int numberOfCells, int targetRank ) {
  assertionEquals( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size(), 1 );

  const int sourceTree = *( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin() );
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,peano4::SplitInstruction{numberOfCells,_configuration->getMode(_state)},targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  _blacklist.triggeredSplit( sourceTree );
  _statistics.incLocalNumberOfSplits();

  _stepsToWaitForNextLoadBalancingDecision = 3;
}


