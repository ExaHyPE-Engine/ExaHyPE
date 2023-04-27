#include "SplitOversizedTree.h"

#include "loadbalancing.h"

#include "peano4/utils/Globals.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "tarch/multicore/Core.h"

#include "peano4/grid/GridStatistics.h"



tarch::logging::Log  toolbox::loadbalancing::SplitOversizedTree::_log( "toolbox::loadbalancing::SplitOversizedTree" );


toolbox::loadbalancing::SplitOversizedTree::SplitOversizedTree(Configuration* configuration, bool deleteInDestructor):
  LoadBalancing( configuration, deleteInDestructor ),
  _roundRobinToken(0) {
  assertion( configuration!=nullptr );
  _state = State::InterRankDistribution;
}


toolbox::loadbalancing::SplitOversizedTree::~SplitOversizedTree() {
}


std::string toolbox::loadbalancing::SplitOversizedTree::toString() const {
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
      << ",target-tree-size=" << getTargetTreeSize()
      << ")";

  return msg.str();
}


void toolbox::loadbalancing::SplitOversizedTree::updateState() {
  if ( _state == State::SwitchedOff ) {
    logDebug( "updateState()", "don't update as we have switched off" );
  }
  else if ( areRanksUnemployed() ) {
    _state = State::InterRankDistribution;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else if ( isInterRankBalancingBad() and _roundRobinToken==tarch::mpi::Rank::getInstance().getRank() and doesLocalTreeViolateThreshold() ) {
    _state = State::InterRankBalancing;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else if ( isIntraRankBalancingBad() and doesLocalTreeViolateThreshold() ) {
    _state = State::IntraRankBalancing;
    logDebug( "updateState()", "switch to state " << ::toString(_state) );
  }
  else {
    _state = State::WaitForRoundRobinToken;

    _roundRobinToken++;
    _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();
  }
}


bool toolbox::loadbalancing::SplitOversizedTree::doesLocalTreeViolateThreshold() const {
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
    bool result = false;

    for (auto p: peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees() ) {
      result |= peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() > getTargetTreeSize();
    }

    return result;
  }
}


void toolbox::loadbalancing::SplitOversizedTree::updateLoadBalancing() {
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
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > getTargetTreeSize()
          and
          _statistics.getLightestRank()!=tarch::mpi::Rank::getInstance().getRank()
        ) {
          logInfo(
            "updateLoadBalancing()",
            "Not all ranks are used. Lightest global rank is rank " << _statistics.getLightestRank()
          );
          triggerSplit(heaviestSpacetree, _statistics.getLightestRank());
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
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > getTargetTreeSize()
        ) {
          logInfo(
            "updateLoadBalancing()",
            "biggest local tree " << heaviestSpacetree << " is too heavy as it hosts " <<
            numberOfLocalUnrefinedCellsOfHeaviestSpacetree << " cells"
          );
          triggerSplit(heaviestSpacetree, _statistics.getLightestRank());
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
        for (auto p: peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees() ) {
          if (
            fitsIntoMemory(_state)
            and
            not _blacklist.isBlacklisted(p)
            and
	    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() > getTargetTreeSize()
          ) {
            triggerSplit(p, tarch::mpi::Rank::getInstance().getRank());
	  }
        }
      }
      break;
    default:
      break;
  }
}


void toolbox::loadbalancing::SplitOversizedTree::finishStep() {
  _statistics.updateGlobalView();
  _blacklist.update();

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange( _state );

  dumpStatistics();
}


void toolbox::loadbalancing::SplitOversizedTree::triggerSplit( int sourceTree, int targetRank ) {
  int currentCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(sourceTree).getNumberOfLocalUnrefinedCells();
  while (currentCells > getTargetTreeSize()) {
    int numberOfSplitCells = currentCells < 2 * getTargetTreeSize() ? currentCells / 2 : getTargetTreeSize();
    bool success = peano4::parallel::SpacetreeSet::getInstance().split(
      sourceTree,
      peano4::SplitInstruction{
        numberOfSplitCells,
	peano4::SplitInstruction::Mode::AggressiveTopDown
      },
      targetRank
    );
    if (not success) {
      logInfo( "triggerSplit()", "wanted to split local rank " << sourceTree << " but failed" );
    }

    _blacklist.triggeredSplit( sourceTree );
    _statistics.incLocalNumberOfSplits();

    currentCells -= getTargetTreeSize();
  }
}


int toolbox::loadbalancing::SplitOversizedTree::getTargetTreeSize() const {
  return _configuration->getMinTreeSize(_state) > 0 ?
         _configuration->getMinTreeSize(_state) :
	 std::max( 1, _statistics.getGlobalNumberOfInnerUnrefinedCells() / tarch::mpi::Rank::getInstance().getNumberOfRanks() / tarch::multicore::Core::getInstance().getNumberOfThreads() );
}



