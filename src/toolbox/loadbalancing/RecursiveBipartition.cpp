#include "RecursiveBipartition.h"

#include "loadbalancing.h"

#include "peano4/utils/Globals.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "tarch/multicore/Core.h"

#include "peano4/grid/GridStatistics.h"



tarch::logging::Log  toolbox::loadbalancing::RecursiveBipartition::_log( "toolbox::loadbalancing::RecursiveBipartition" );


toolbox::loadbalancing::RecursiveBipartition::RecursiveBipartition(Configuration* configuration, bool deleteInDestructor):
  LoadBalancing( configuration, deleteInDestructor ),
  _stepsToWaitForNextLoadBalancingDecision(0),
  _roundRobinToken(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankBalancing;
}


toolbox::loadbalancing::RecursiveBipartition::~RecursiveBipartition() {
}


std::string toolbox::loadbalancing::RecursiveBipartition::toString() const {
  std::ostringstream msg;

  msg << "(state=" << ::toString( _state )
      << ",statistics=" << _statistics.toString()
      << ",round-robin-token=" << _roundRobinToken 
      << ",steps-to-wait-for-next-load-balancing-decision=" << _stepsToWaitForNextLoadBalancingDecision
      << ",blacklist=" << _blacklist.toString()
      << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
      << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
      << ",#local-trees=" << peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() 
      << ",is-local-balancing-bad=" << isLocalBalancingBad()
      << ")";

  return msg.str();
}


bool toolbox::loadbalancing::RecursiveBipartition::doesRankViolateBalancingCondition() const {
  int localCells = 0;
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  for (auto p: idsOfLocalSpacetrees) {
    localCells += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
  }

  const double optimalCellCount = static_cast<double>(_statistics.getGlobalNumberOfInnerUnrefinedCells())
                         / tarch::mpi::Rank::getInstance().getNumberOfRanks();

  const double illbalancing   = (static_cast<double>(localCells)-optimalCellCount) / optimalCellCount;
  const double balancingRatio = _configuration->getWorstCaseBalancingRatio(_state);
  bool result = (localCells < _statistics.getGlobalNumberOfInnerUnrefinedCells()) and (illbalancing > 1.0 - balancingRatio);

  if (result) {
    logInfo( "doesRankViolateBalancingCondition()", "rank does violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _statistics.getGlobalNumberOfInnerUnrefinedCells() << ", local cells=" << localCells << ", optimal cell count=" << optimalCellCount << ")" );
  }
  else {
    logDebug( "doesRankViolateBalancingCondition()", "rank does not violate balancing as we have ill-balancing of " << illbalancing << " (global cells=" << _statistics.getGlobalNumberOfInnerUnrefinedCells() << ", local cells=" << localCells << ", optimal cell count=" << optimalCellCount << ")" );
  }

  return result;
}


void toolbox::loadbalancing::RecursiveBipartition::updateState() {
  switch (_state) {
    case State::InterRankDistribution:
      assertion1(false, ::toString(_state) );
      break;
    case State::IntraRankDistribution:
      assertion1(false, ::toString(_state) );
      break;
    case State::WaitForRoundRobinToken:
      {
        _roundRobinToken++;
        _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();

        if (_roundRobinToken==tarch::mpi::Rank::getInstance().getRank()) {
          _state = State::InterRankBalancing;
          logDebug( "updateState()", "switch to state " << ::toString(_state) );
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
          logDebug( "updateState()", "switch to state " << ::toString(_state) );
        }
        else {
          _state = State::WaitForRoundRobinToken;
          logDebug( "updateState()", "switch to state " << ::toString(_state) );
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
          logDebug( "updateState()", "switch to state " << ::toString(_state) );
        }
      }
      break;
    case State::Stagnation:
      if ( getWeightOfHeaviestLocalSpacetree()==-1) {
        _state = State::Stagnation;
        logDebug( "updateState()", "switch to state " << ::toString(_state) );
      }
      else {
        _state = State::WaitForRoundRobinToken;
        logDebug( "updateState()", "switch to state " << ::toString(_state) );
      }
      break;
    case State::SwitchedOff:
      break;
  }
}


std::string toolbox::loadbalancing::RecursiveBipartition::toString( Action action ) {
  switch ( action ) {
    case Action::None:
      return "none";
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


toolbox::loadbalancing::RecursiveBipartition::Action toolbox::loadbalancing::RecursiveBipartition::getAction() const {
  if (_stepsToWaitForNextLoadBalancingDecision>0) {
      // @todo Debug
    logInfo(
      "getAction()",
      "shall wait for another " << _stepsToWaitForNextLoadBalancingDecision << " step(s) before we rebalance"
    );
    return Action::None;
  }

  switch (_state) {
    case State::InterRankDistribution:
      assertion1(false, ::toString(_state) );
      break;
    case State::IntraRankDistribution:
      assertion1(false, ::toString(_state) );
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
    default:
      break;
  }

  return Action::None;
}


bool toolbox::loadbalancing::RecursiveBipartition::isLocalBalancingBad() const {
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


void toolbox::loadbalancing::RecursiveBipartition::updateLoadBalancing() {
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
    case Action::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
        ) {
          logInfo(
            "updateLoadBalancing()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMinTreeSize(_state)} );
 
          logInfo(
            "updateLoadBalancing()",
            "lightest global rank is rank " << _statistics.getLightestRank() << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _statistics.getLightestRank());
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
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          _statistics.getLightestRank()!=tarch::mpi::Rank::getInstance().getRank()
        ) {
          logInfo(
            "updateLoadBalancing()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2, _configuration->getMinTreeSize(_state)} );
          logInfo(
            "updateLoadBalancing()",
            "lightest global rank is rank " << _statistics.getLightestRank() << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _statistics.getLightestRank());
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
          not _blacklist.isBlacklisted(heaviestSpacetree)
        ) {
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2, _configuration->getMinTreeSize(_state)} );
          logInfo(
            "updateLoadBalancing()",
            "split tree on local rank and assign it " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
        }
        else if ( not _blacklist.isBlacklisted (heaviestSpacetree) ) {
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


void toolbox::loadbalancing::RecursiveBipartition::finishStep() {
  _statistics.updateGlobalView();
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


void toolbox::loadbalancing::RecursiveBipartition::triggerSplit( int sourceTree, int numberOfCells, int targetRank ) {
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,numberOfCells,targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  _blacklist.triggeredSplit( sourceTree );
  _statistics.incLocalNumberOfSplits();

  _stepsToWaitForNextLoadBalancingDecision = 3;
}

