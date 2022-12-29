#include "ParticleSpecies.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

#include <limits>


tarch::multicore::BooleanSemaphore swift2::ParticleSpecies::_semaphore;
tarch::logging::Log                swift2::ParticleSpecies::_log( "swift2::ParticleSpecies" );


swift2::ParticleSpecies::ParticleSpecies():
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStamp(0.0), // std::numeric_limits<double>::max()
  _maxTimeStamp(0.0),
  _minCutOffRadius(std::numeric_limits<double>::max()),
  _maxCutOffRadius(0.0),
  _minVelocity(std::numeric_limits<double>::max()),
  _maxVelocity(0.0),
  _rerunPreviousGridSweep(false) {
}


void swift2::ParticleSpecies::setTimeStamp( double t, bool reduceInsteadOfOverwrite ) {
  setTimeStamp(t,t,reduceInsteadOfOverwrite);
}


void swift2::ParticleSpecies::setTimeStamp( double tMin, double tMax, bool reduceInsteadOfOverwrite ) {
  tarch::multicore::Lock lock(_semaphore);
  if (reduceInsteadOfOverwrite) {
    _minTimeStamp = std::min( _minTimeStamp, tMin );
    _maxTimeStamp = std::max( _maxTimeStamp, tMax );
  }
  else {
    _minTimeStamp = tMin;
    _maxTimeStamp = tMax;
  }
}


void swift2::ParticleSpecies::setTimeStepSize( double dt, bool reduceInsteadOfOverwrite ) {
  setTimeStepSize(dt,dt,reduceInsteadOfOverwrite);
}


bool swift2::ParticleSpecies::rerunPreviousGridSweep() const {
  return _rerunPreviousGridSweep;
}


void swift2::ParticleSpecies::clearRerunPreviousGridSweepFlag() {
  _rerunPreviousGridSweep = false;
}


void swift2::ParticleSpecies::setRerunPreviousGridSweep() {
  tarch::multicore::Lock lock(_semaphore);
  _rerunPreviousGridSweep = true;
}



void swift2::ParticleSpecies::setTimeStepSize( double dtMin, double dtMax, bool reduceInsteadOfOverwrite ) {
  tarch::multicore::Lock lock(_semaphore);
  if (reduceInsteadOfOverwrite) {
    _minTimeStepSize = std::min( _minTimeStepSize, dtMin );
    _maxTimeStepSize = std::max( _maxTimeStepSize, dtMax );
  }
  else {
    _minTimeStepSize = dtMin;
    _maxTimeStepSize = dtMax;
  }
}


double swift2::ParticleSpecies::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double swift2::ParticleSpecies::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


double swift2::ParticleSpecies::getMinTimeStamp() const {
  return _minTimeStamp;
}


double swift2::ParticleSpecies::getMaxTimeStamp() const {
  return _maxTimeStamp;
}


double swift2::ParticleSpecies::getMinCutOffRadius() const {
  return _minCutOffRadius;
}


double swift2::ParticleSpecies::getMaxCutOffRadius() const {
  return _maxCutOffRadius;
}


double swift2::ParticleSpecies::getMinVelocity() const {
  return _minVelocity;
}


double swift2::ParticleSpecies::getMaxVelocity() const {
  return _maxVelocity;
}


void swift2::ParticleSpecies::setVelocity(double vMin, double vMax, double rMin, double rMax) {
  tarch::multicore::Lock lock(_semaphore);
  _minVelocity     = std::min( _minVelocity, vMin );
  _maxVelocity     = std::max( _maxVelocity, vMax );
  _minCutOffRadius = std::min( _minCutOffRadius, rMin );
  _maxCutOffRadius = std::max( _maxCutOffRadius, rMax );
  logDebug( "setVelocity()", vMin << "x" << vMax );
}


void swift2::ParticleSpecies::clearCutOffRadius() {
  _minCutOffRadius = std::numeric_limits<double>::max();
  _maxCutOffRadius = 0.0;
}


void swift2::ParticleSpecies::clearVelocity() {
  _minVelocity = std::numeric_limits<double>::max();
  _maxVelocity = 0.0;
}


void swift2::ParticleSpecies::clearTimeStampAndTimeStepSize() {
  _minTimeStepSize = std::numeric_limits<double>::max();
  _maxTimeStepSize = 0.0;

  _minTimeStamp = std::numeric_limits<double>::max();
  _maxTimeStamp = 0.0;
}


void swift2::ParticleSpecies::startTimeStep() {
}


void swift2::ParticleSpecies::finishTimeStep() {

}


void swift2::ParticleSpecies::allReduce() {
  #ifdef Parallel

  double localMinTimeStepSize  = _minTimeStepSize;
  double localMaxTimeStepSize  = _maxTimeStepSize;

  double localMinTimeStamp     = _minTimeStamp;
  double localMaxTimeStamp     = _maxTimeStamp;

  double localMinCutOffRadius  = _minCutOffRadius;
  double localMaxCutOffRadius  = _maxCutOffRadius;

  double localMinVelocity      = _minVelocity;
  double localMaxVelocity      = _maxVelocity;

  int localRerunPreviousGridSweep  = _rerunPreviousGridSweep ? 1 : 0;
  int globalRerunPreviousGridSweep = 0;

  tarch::mpi::Rank::getInstance().allReduce(
    &localMinTimeStepSize,
    &_minTimeStepSize,
    1, MPI_DOUBLE,
    MPI_MIN,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMaxTimeStepSize,
    &_maxTimeStepSize,
    1, MPI_DOUBLE,
    MPI_MAX,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMinTimeStamp,
    &_minTimeStamp,
    1, MPI_DOUBLE,
    MPI_MIN,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMaxTimeStamp,
    &_maxTimeStamp,
    1, MPI_DOUBLE,
    MPI_MAX,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMinCutOffRadius,
    &_minCutOffRadius,
    1, MPI_DOUBLE,
    MPI_MIN,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMaxCutOffRadius,
    &_maxCutOffRadius,
    1, MPI_DOUBLE,
    MPI_MAX,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMinVelocity,
    &_minVelocity,
    1, MPI_DOUBLE,
    MPI_MIN,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localMaxVelocity,
    &_maxVelocity,
    1, MPI_DOUBLE,
    MPI_MAX,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &localRerunPreviousGridSweep,
    &globalRerunPreviousGridSweep,
    1, MPI_INT,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  _rerunPreviousGridSweep = globalRerunPreviousGridSweep>0;

  #endif
}
