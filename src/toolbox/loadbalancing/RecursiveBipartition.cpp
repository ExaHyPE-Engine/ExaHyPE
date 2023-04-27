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
  _roundRobinToken(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankDistribution;
}


toolbox::loadbalancing::RecursiveBipartition::~RecursiveBipartition() {
}


std::string toolbox::loadbalancing::RecursiveBipartition::toString() const {
  std::ostringstream msg;

  msg << "(state=" << ::toString( _state )
      << ",statistics=" << _statistics.toString()
      << ",round-robin-token=" << _roundRobinToken 
      << ",blacklist=" << _blacklist.toString()
      << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
      << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
      << ",#local-trees=" << peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() 
      << ",is-inter-rank-balancing-bad=" << isInterRankBalancingBad()
      << ",is-intra-rank-balancing-bad=" << isIntraRankBalancingBad()
      << ")";

  return msg.str();
}


bool toolbox::loadbalancing::RecursiveBipartition::isInterRankBalancingBad() const {
  if ( not _statistics.hasConsistentViewOfWorld()) {
    return false;
  }
  else if ( _statistics.getGlobalNumberOfTrees() < tarch::mpi::Rank::getInstance().getNumberOfRanks() ) {
    return true;
  }
  else if ( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() >= _configuration->getMaxLocalTreesPerRank(_state) ) {
    return false;
  }
  else {
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
}


void toolbox::loadbalancing::RecursiveBipartition::updateState() {
  if ( _state == State::SwitchedOff ) {
    logDebug( "updateState()", "don't update as we have switched off" );
  }
  else if ( areRanksUnemployed() ) {
    _state = State::InterRankDistribution;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else if ( isInterRankBalancingBad() and _roundRobinToken==tarch::mpi::Rank::getInstance().getRank()) {
    _state = State::InterRankBalancing;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else if ( isIntraRankBalancingBad() ) {
    _state = State::IntraRankBalancing;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else {
    _state = State::WaitForRoundRobinToken;

    _roundRobinToken++;
    _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();
  }
}


bool toolbox::loadbalancing::RecursiveBipartition::areRanksUnemployed() const {
  return _statistics.getMinimumOfMaximumOfTreesPerRank() == 0
     and _statistics.getGlobalNumberOfTrees() > 0;
}


bool toolbox::loadbalancing::RecursiveBipartition::isIntraRankBalancingBad() const {
  if ( not _statistics.hasConsistentViewOfWorld()) {//
    return false;
  }
  else if ( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()==0 ) {
    return false;
  }
  else if ( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()==1 ) {
    return true;
  }
  else if ( peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() >= _configuration->getMaxLocalTreesPerRank(_state) ) {
    return false;
  }
  else {
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
}


void toolbox::loadbalancing::RecursiveBipartition::updateLoadBalancing() {
  // @todo
//  #if PeanoDebug>0
  logInfo( "updateLoadBalancing()", "load balancing's state=" << toString() );
//  #endif

  switch ( _state ) {
    case State::InterRankDistribution:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree( _configuration->getWorstCaseBalancingRatio(_state) );
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
          and
          _statistics.getLightestRank()!=tarch::mpi::Rank::getInstance().getRank()
        ) {
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2,_configuration->getMinTreeSize(_state)} );

          logInfo(
            "updateLoadBalancing()",
            "Not all ranks are used. Lightest global rank is rank " << _statistics.getLightestRank() << ", so assign this rank " << cellsPerCore << " cell(s)"
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, _statistics.getLightestRank());
        }
        else if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
          logInfo(
            "updateLoadBalancing()",
            "can't split heaviest tree " << heaviestSpacetree << " even though this tree is too heavy"
          );
        }
      }
      break;
    case State::InterRankBalancing:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
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
        else if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
          logInfo(
            "updateLoadBalancing()",
            "can't split heaviest tree " << heaviestSpacetree << " and fork off to remote rank as this tree is on the blacklist or split does not fit into local memory anymore"
          );
        }
      }
      break;
    case State::IntraRankBalancing:
      {
        int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
        int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
        if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
        ) {
          int cellsPerCore      = std::max( {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree/2, _configuration->getMinTreeSize(_state)} );
          logInfo(
            "updateLoadBalancing()",
            "split tree on local rank and assign it " << cellsPerCore << " cell(s)" <<
            toString()
          );
          triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
        }
        else if (heaviestSpacetree!=NoHeaviestTreeAvailable) {
          logInfo(
            "updateLoadBalancing()",
            "can't split heaviest tree " << heaviestSpacetree << " on local rank as this tree is on the blacklist or split does not fit into local memory anymore"
          );
        }
      }
      break;
    default:
      break;
  }
}


void toolbox::loadbalancing::RecursiveBipartition::finishStep() {
  _statistics.updateGlobalView();
  _blacklist.update();

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange( _state );

  dumpStatistics();
}


void toolbox::loadbalancing::RecursiveBipartition::triggerSplit( int sourceTree, int numberOfCells, int targetRank ) {
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(sourceTree,peano4::SplitInstruction{numberOfCells,peano4::SplitInstruction::Mode::AggressiveTopDown},targetRank);
  if (not success) {
    logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
  }

  _blacklist.triggeredSplit( sourceTree );
  _statistics.incLocalNumberOfSplits();
}

