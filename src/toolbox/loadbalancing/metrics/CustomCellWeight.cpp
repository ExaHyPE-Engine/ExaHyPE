#include "CustomCellWeight.h"

#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/multicore/Lock.h"

#include "toolbox/loadbalancing/AbstractLoadBalancing.h"


tarch::multicore::BooleanSemaphore  toolbox::loadbalancing::metrics::CustomCellWeight::_semaphore;
tarch::logging::Log                 toolbox::loadbalancing::metrics::CustomCellWeight::_log( "toolbox::loadbalancing::metrics::CustomCellWeight" );
std::map<int, double>               toolbox::loadbalancing::metrics::CustomCellWeight::_currentCellWeights;
std::map<int, double>               toolbox::loadbalancing::metrics::CustomCellWeight::_previousCellWeights;


toolbox::loadbalancing::metrics::CustomCellWeight::CustomCellWeight() {
}


std::string toolbox::loadbalancing::metrics::CustomCellWeight::toString() const {
  return CostMetrics::toString( "custom-cell-weight" );
}


void toolbox::loadbalancing::metrics::CustomCellWeight::updateGlobalView() {
  _previousCellWeights.clear();
  _previousCellWeights.insert( _currentCellWeights.begin(), _currentCellWeights.end() );

  _currentCellWeights.clear();

  _localRankWeight = 0;
  for (auto& p: _previousCellWeights) {
      _localRankWeight += p.second;
  }

  CostMetrics::updateGlobalView();
}


double toolbox::loadbalancing::metrics::CustomCellWeight::getCostOfLocalTree(int spacetreeNumber) const {
  if ( _previousCellWeights.count(spacetreeNumber)==0 ) {
    return 0.0;
  }
  else {
    return _previousCellWeights.at(spacetreeNumber);
  }
}


void toolbox::loadbalancing::metrics::CustomCellWeight::logCellWeight( int spacetreeNumber, double weight ) {
  tarch::multicore::Lock lock( _semaphore );
  if ( _currentCellWeights.count(spacetreeNumber)==0 ) {
    _currentCellWeights.insert( std::pair<int,double>(spacetreeNumber,0.0) );
  }
  lock.free();

  _currentCellWeights[spacetreeNumber] += weight;
}


