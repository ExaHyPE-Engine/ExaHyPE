// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"


namespace swift2 {
  class ParticleSpecies;
}


/**
 * Represent one type (species) of particles
 *
 * Every particle type (class) has a static species, i.e. some data tied to the
 * class rather than each individual object. These species data hold information
 * such as global statistics.
 *
 *
 * ## Thread-safety
 *
 * The class is completely thread-safe.
 *
 *
 */
class swift2::ParticleSpecies {
  private:
    static tarch::multicore::BooleanSemaphore _semaphore;

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStamp;
    double _maxTimeStamp;

    double _minCutOffRadius;
    double _maxCutOffRadius;

    double _maxVelocity;
  public:
    ParticleSpecies() = default;
    ParticleSpecies( const ParticleSpecies& copy ) = delete;

    double getMinTimeStepSize() const;
    double getMaxTimeStepSize() const;

    double getMinTimeStamp() const;
    double getMaxTimeStamp() const;

    double getMinCutOffRadius() const;
    double getMaxCutOffRadius() const;

    void setMaxVelocity(double value);

    void startTimeStep();
    void finishTimeStep();
};
