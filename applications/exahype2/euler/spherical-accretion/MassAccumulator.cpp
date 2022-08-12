#include "MassAccumulator.h"

#include "tarch/multicore/Lock.h"

#include <cmath>


tarch::logging::Log  applications::exahype2::euler::sphericalaccretion::MassAccumulator::_log( "applications::exahype2::euler::sphericalaccretion::MassAccumulator" );


applications::exahype2::euler::sphericalaccretion::MassAccumulator::MassAccumulator(double shellWidth, double threshold):
  _shellWidth(shellWidth),
  _threshold(threshold) {
}


int applications::exahype2::euler::sphericalaccretion::MassAccumulator::getBucket( double radius ) const {
  return (int)(std::floor( radius / _shellWidth ));
}


void applications::exahype2::euler::sphericalaccretion::MassAccumulator::addMass(double mass, double radius) {
  tarch::multicore::Lock lock( _semaphore );
  int bucket = getBucket(radius);
  while (_currentMass.size()<=bucket) {
    _currentMass.push_back(0.0);
  }
  _currentMass[bucket] += mass;
}


double applications::exahype2::euler::sphericalaccretion::MassAccumulator::getMass(double radius) const {
  int bucket = getBucket(radius);
  if (_previousMassAccumulated.empty()) {
    return 0.0;
  }
  else if (_previousMassAccumulated.size()<=bucket) {
    return _previousMassAccumulated.at(_previousMassAccumulated.size()-1);
  }
  else {
    return _previousMassAccumulated.at(bucket);
  }
}


void applications::exahype2::euler::sphericalaccretion::MassAccumulator::finishAccumulation() {
  #ifdef Parallel
  assertion(false); // muss den Vektor reduzieren
  tarch::mpi::Rank::getInstance().allReduce(
    m_tot,
    global_m_tot,
    sample_number, MPI_DOUBLE,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    cell_tot,
    global_cell_tot,
    sample_number, MPI_DOUBLE,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  #endif
  _previousMassAccumulated.clear();
  _previousMassAccumulated.insert( _previousMassAccumulated.end(), _currentMass.begin(), _currentMass.end() );
  _currentMass.clear();

  int maxShell = 1;
  for (int i=0; i<_previousMassAccumulated.size(); i++) {
    if (i>0) {
      _previousMassAccumulated[i] += _previousMassAccumulated[i-1];
      if ( _previousMassAccumulated[i] / _previousMassAccumulated[i-1] > 1.0+_threshold) {
        maxShell++;
      }
    }
  }
  logInfo( "finishAccumulation()", "roll over " << maxShell << " out of " << _previousMassAccumulated.size() << " buckets" );
  _previousMassAccumulated.resize(maxShell);


  double currentShellWidth = 0.0;
  for( auto& p: _previousMassAccumulated ) {
    currentShellWidth += _shellWidth;
    logInfo( "finishAccumulation()", "bucket (r<" << currentShellWidth << ") = " << p );
  }
}
