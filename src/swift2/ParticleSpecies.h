// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"


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
    static tarch::logging::Log                 _log;
    static tarch::multicore::BooleanSemaphore  _semaphore;

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStamp;
    double _maxTimeStamp;

    double _minCutOffRadius;
    double _maxCutOffRadius;

    double _minVelocity;
    double _maxVelocity;
  public:
    /**
     * Initialise all data with the data corresponding to a clear, i.e.
     * from where they can be properly reduced. The only exception is
     * the time stamp. Both the maximum and minimum time step are set
     * to zero. If we set the max time stamp to inf, the code would
     * immediately terminate, as it thinks at least one particle has
     * reached the final time stamp.
     */
    ParticleSpecies();
    ParticleSpecies( const ParticleSpecies& copy ) = delete;

    double getMinTimeStepSize() const;
    double getMaxTimeStepSize() const;

    double getMinTimeStamp() const;
    double getMaxTimeStamp() const;

    double getMinCutOffRadius() const;
    double getMaxCutOffRadius() const;

    double getMinVelocity() const;
    double getMaxVelocity() const;

    /**
     * Set both min and max time stamp.
     */
    void setTimeStamp( double t, bool reduceInsteadOfOverwrite=true );
    void setTimeStamp( double tMin, double tMax, bool reduceInsteadOfOverwrite=true );
    void setTimeStepSize( double dt, bool reduceInsteadOfOverwrite=true );
    void setTimeStepSize( double dtMin, double dtMax, bool reduceInsteadOfOverwrite=true );

    /**
     * Set the maximum velocity that has been observed in a cell. This routine
     * takes the maximum velocity as observed so far and updates it if value is
     * bigger.
     *
     * The routine is thread-safe.
     */
    void setVelocity(double vMin, double vMax, double rMin, double rMax);

    /**
     * This routine is automatically called for each and every species by the
     * GlobalState object.
     */
    void startTimeStep();

    /**
     *
     * This routine is automatically called for each and every species by the
     * GlobalState object.
     */
    void finishTimeStep();

    void clearCutOffRadius();
    void clearVelocity();
    void clearTimeStampAndTimeStepSize();
};
