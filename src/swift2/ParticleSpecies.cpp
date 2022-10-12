#include "ParticleSpecies.h"

#include "tarch/multicore/Lock.h"


tarch::multicore::BooleanSemaphore swift2::ParticleSpecies::_semaphore;


double swift2::ParticleSpecies::getMinTimeStepSize() const {
  return 10.0;
  return _minTimeStepSize;
}


double swift2::ParticleSpecies::getMaxTimeStepSize() const {
  return 10.0;
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


void swift2::ParticleSpecies::setMaxVelocity(double value) {
  tarch::multicore::Lock lock(_semaphore);
  _maxVelocity = std::max( _maxVelocity, value );
}


void swift2::ParticleSpecies::startTimeStep() {
  _maxVelocity = 0.0;
}


void swift2::ParticleSpecies::finishTimeStep() {

}


