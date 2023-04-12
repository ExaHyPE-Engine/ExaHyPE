#include "LoadBalancing.h"



tarch::logging::Log  toolbox::loadbalancing::LoadBalancing::_log( "toolbox::loadbalancing::LoadBalancing" );


toolbox::loadbalancing::LoadBalancing::LoadBalancing(Configuration* configuration):
  _state( State::Undefined ),
  _configuration(configuration) {
  assertion(configuration!=nullptr);
}


toolbox::loadbalancing::LoadBalancing::~LoadBalancing() {
  delete _configuration;
}


std::string toolbox::loadbalancing::LoadBalancing::toString() const {
  std::ostringstream msg;

  msg << "(state=" << ::toString( _state )
      << ",statistics=" << _statistics.toString()
      << ",blacklist=" << _blacklist.toString()
      << ")";

  return msg.str();
}


bool toolbox::loadbalancing::LoadBalancing::hasSplitRecently() const {
  return _statistics.getNumberOfStateUpdatesWithoutAnySplit()<3;
}


void toolbox::loadbalancing::LoadBalancing::enable( bool value ) {
  if (value and _state==State::SwitchedOff) {
    _state = State::InterRankBalancing;
    logInfo( "enable()", "switched load balancing on (again) and therefore change to state " << ::toString(_state) );
  }
  else if (not value and _state != State::SwitchedOff) {
    logInfo( "enable()", "switched load balancing off" );
    _state = State::SwitchedOff;
  }
}


bool toolbox::loadbalancing::LoadBalancing::isEnabled(bool globally) const {
  logDebug( "isEnabled(bool)", toString() );
  return globally ? (_statistics.getGlobalNumberOfRanksWithEnabledLoadBalancing()>0) : (_state != State::SwitchedOff);
}


int toolbox::loadbalancing::LoadBalancing::getGlobalNumberOfTrees() const {
  return _statistics.getGlobalNumberOfTrees();
}


bool toolbox::loadbalancing::LoadBalancing::fitsIntoMemory(State state) const {
  // This is a magic paramter with the 2. You might still run out of memory if you refine while you
  // rebalance.
  if ( _configuration->makeSplitDependOnMemory(_state) ) {
    int worstCaseEstimateForSizeOfSpacetree = 2 * tarch::getMemoryUsage( tarch::MemoryUsageFormat::MByte );
    return tarch::getFreeMemory( tarch::MemoryUsageFormat::MByte ) >= worstCaseEstimateForSizeOfSpacetree;
  }
  else return true;
}


void toolbox::loadbalancing::LoadBalancing::finishSimulation() {
  _statistics.waitForGlobalStatisticsExchange();
}
