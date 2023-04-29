#include "AbstractLoadBalancing.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log  toolbox::loadbalancing::AbstractLoadBalancing::_log( "toolbox::loadbalancing::AbstractLoadBalancing" );


toolbox::loadbalancing::AbstractLoadBalancing::AbstractLoadBalancing(Configuration* configuration, bool deleteInDestructor):
  _state( State::Undefined ),
  _configuration(configuration),
  _deleteInDestructor(deleteInDestructor) {
}


toolbox::loadbalancing::AbstractLoadBalancing::~AbstractLoadBalancing() {
  if (_deleteInDestructor) {
    delete _configuration;
  }
}


std::string toolbox::loadbalancing::AbstractLoadBalancing::toString() const {
  std::ostringstream msg;

  msg << "(state=" << ::toString( _state )
      << ",statistics=" << _statistics.toString()
      << ",blacklist=" << _blacklist.toString()
      << ")";

  return msg.str();
}


bool toolbox::loadbalancing::AbstractLoadBalancing::hasSplitRecently() const {
  return _statistics.getNumberOfStateUpdatesWithoutAnySplit()<3;
}


void toolbox::loadbalancing::AbstractLoadBalancing::enable( bool value ) {
  if (value and _state==State::SwitchedOff) {
    _state = State::InterRankBalancing;
    logInfo( "enable()", "switched load balancing on (again) and therefore change to state " << ::toString(_state) );
  }
  else if (not value and _state != State::SwitchedOff) {
    logInfo( "enable()", "switched load balancing off" );
    _state = State::SwitchedOff;
  }
}


bool toolbox::loadbalancing::AbstractLoadBalancing::isEnabled(bool globally) const {
  logDebug( "isEnabled(bool)", toString() );
  return globally ? (_statistics.getGlobalNumberOfRanksWithEnabledLoadBalancing()>0) : (_state != State::SwitchedOff);
}


bool toolbox::loadbalancing::AbstractLoadBalancing::hasStagnated() const {
  return _state == State::Stagnation or _state == State::SwitchedOff;
}


int toolbox::loadbalancing::AbstractLoadBalancing::getGlobalNumberOfTrees() const {
  return _statistics.getGlobalNumberOfTrees();
}


bool toolbox::loadbalancing::AbstractLoadBalancing::fitsIntoMemory(State state) const {
  // This is a magic paramter with the 2. You might still run out of memory if you refine while you
  // rebalance.
  if ( _configuration->makeSplitDependOnMemory(_state) ) {
    int worstCaseEstimateForSizeOfSpacetree = 2 * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
    return tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) >= worstCaseEstimateForSizeOfSpacetree;
  }
  else return true;
}


void toolbox::loadbalancing::AbstractLoadBalancing::finishSimulation() {
  _statistics.waitForGlobalStatisticsExchange();
}


bool toolbox::loadbalancing::AbstractLoadBalancing::isInterRankBalancingBad() const {
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


bool toolbox::loadbalancing::AbstractLoadBalancing::isIntraRankBalancingBad() const {
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


bool toolbox::loadbalancing::AbstractLoadBalancing::areRanksUnemployed() const {
  return _statistics.getMinimumOfMaximumOfTreesPerRank() == 0
     and _statistics.getGlobalNumberOfTrees() > 0;
}


