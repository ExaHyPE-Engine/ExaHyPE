#include "MassAccumulator.h"

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

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


double applications::exahype2::euler::sphericalaccretion::MassAccumulator::getMass_piecewiseConstantInterpolation(double radius) const {
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


double applications::exahype2::euler::sphericalaccretion::MassAccumulator::getMass_linearInterpolation(double radius) const {
  int bucket = getBucket(radius);

  if (_previousMassAccumulated.empty()) {
    return 0.0;
  }
  else if (_previousMassAccumulated.size()<=bucket) {
    return _previousMassAccumulated.at(_previousMassAccumulated.size()-1);
  }
  else {
    int    leftBucket  = bucket;
    int    rightBucket = bucket;
    double rightWeight = 1.0;

    if ( radius-bucket*_shellWidth < _shellWidth/2.0 and bucket>0 ) {
      leftBucket--;
      rightWeight = (radius-(leftBucket+0.5)*_shellWidth) / _shellWidth;
    }
    else if ( radius-bucket*_shellWidth < _shellWidth/2.0 and bucket==0 ) {
      rightWeight = 1.0;
    }
    else if (radius-bucket*_shellWidth >= _shellWidth/2.0 and bucket<_previousMassAccumulated.size()-1) {
      rightBucket++;
      rightWeight = (radius-(leftBucket+0.5)*_shellWidth) / _shellWidth;
    }
    else if (radius-bucket*_shellWidth >= _shellWidth/2.0 and bucket==_previousMassAccumulated.size()-1) {
      rightWeight = 0.0;
    }
    else {
      assertion(false);
    }

    assertion6( tarch::la::greaterEquals( rightWeight,0.0 ), rightWeight, leftBucket, rightBucket, bucket, radius, _shellWidth);
    assertion6( tarch::la::smallerEquals( rightWeight,1.0 ), rightWeight, leftBucket, rightBucket, bucket, radius, _shellWidth);

    return rightWeight * _previousMassAccumulated.at(rightBucket) + (1.0-rightWeight) * _previousMassAccumulated.at(leftBucket);
  }
}


void applications::exahype2::euler::sphericalaccretion::MassAccumulator::finishAccumulation() {
  #ifdef Parallel
  int globalMaxNumberOfBuckets = 0;
  int localMaxNumberOfBuckets  = _currentMass.size();
  tarch::mpi::Rank::getInstance().allReduce(
    &localMaxNumberOfBuckets,
    &globalMaxNumberOfBuckets,
    1, MPI_INT,
    MPI_MAX,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  assertion( globalMaxNumberOfBuckets<=_currentMass.size() );
  while (globalMaxNumberOfBuckets<_currentMass.size()) {
    _currentMass.push_back( 0.0 );
  }

  double* globalBucketValues = new double[globalMaxNumberOfBuckets];
  double* localBucketValues  = new double[globalMaxNumberOfBuckets];
  for (int bucket=0; bucket<globalMaxNumberOfBuckets; bucket++) {
    localBucketValues[bucket] = _currentMass[bucket];
  }

  tarch::mpi::Rank::getInstance().allReduce(
    &localBucketValues,
    &globalBucketValues,
    globalMaxNumberOfBuckets, 
    MPI_DOUBLE,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  for (int bucket=0; bucket<globalMaxNumberOfBuckets; bucket++) {
    _currentMass[bucket] = globalBucketValues[bucket];
  }
  delete[] globalBucketValues;
  delete[] localBucketValues;
  #endif

  logInfo( "finishAccumulation()", "old relevant radius equals r=" << getMaxRelevantRadius() );

  _previousMassAccumulated.clear();
  _previousMassAccumulated.insert( _previousMassAccumulated.end(), _currentMass.begin(), _currentMass.end() );
  _currentMass.clear();

  // accumulate
  for (int i=0; i<_previousMassAccumulated.size(); i++) {
    if (i>0) {
      _previousMassAccumulated[i] += _previousMassAccumulated[i-1];
    }
  }

  int largestRelevantShell = 0;
  for (int i=0; i<_previousMassAccumulated.size()-1; i++) {
    if (
      not tarch::la::equals( _previousMassAccumulated[i], 0.0 )
      and
      std::abs(_previousMassAccumulated[i+1] / _previousMassAccumulated[i]) >= 1.0+_threshold
    ) {
      largestRelevantShell = std::max(largestRelevantShell,i+1);
    }
  }
  largestRelevantShell = std::max(0,std::min(largestRelevantShell+1,static_cast<int>(_previousMassAccumulated.size())));
  logInfo( "finishAccumulation()", "roll over " << largestRelevantShell << " out of " << _previousMassAccumulated.size() << " buckets (remaining buckets make no significant contribution)" );
  _previousMassAccumulated.resize( largestRelevantShell );


  double currentShellWidth = 0.0;
  for( auto& p: _previousMassAccumulated ) {
    currentShellWidth += _shellWidth;
    logInfo( "finishAccumulation()", "bucket (r<=" << currentShellWidth << ") = " << p );
  }
  logInfo( "finishAccumulation()", "new relevant radius equals r=" << getMaxRelevantRadius() );
}


double applications::exahype2::euler::sphericalaccretion::MassAccumulator::getMaxRelevantRadius() const {
  return _previousMassAccumulated.empty() ? std::numeric_limits<double>::max() : (_previousMassAccumulated.size()+1.0) * _shellWidth;
}


double applications::exahype2::euler::sphericalaccretion::MassAccumulator::getTotalMass() const {
  return _previousMassAccumulated.back();
}

