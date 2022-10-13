#include "ParticleSpecies.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"


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
  _maxVelocity(0.0) {
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


