// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

namespace swift2 {
  class ParticleSpecies;
} // namespace swift2

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
  static tarch::logging::Log                _log;
  static tarch::multicore::BooleanSemaphore _semaphore;

  double _minTimeStepSize;
  double _maxTimeStepSize;

  double _minTimeStamp;
  double _maxTimeStamp;

  double _minSearchRadius;
  double _maxSearchRadius;

  double _minVelocity;
  double _maxVelocity;

  bool _rerunPreviousGridSweep;

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
  ParticleSpecies(const ParticleSpecies& copy) = delete;

  double getMinTimeStepSize() const;
  double getMaxTimeStepSize() const;

  double getMinTimeStamp() const;
  double getMaxTimeStamp() const;

  double getMinSearchRadius() const;
  double getMaxSearchRadius() const;

  double getMinVelocity() const;
  double getMaxVelocity() const;

  /**
   * Query species whether the last grid sweep/update sweep has to be
   * repeated. This happens if the search radius changes for example.
   */
  bool rerunPreviousGridSweep() const;

  /**
   * Set the rerun flag back to false
   *
   * This operation has to be called explicitly in your code, i.e. we do not
   * invoke it automatically. A canonical location to call it is the
   * prepare_traversal_kernel event of the corresponding AlgorithmStep of
   * your species.
   */
  void clearRerunPreviousGridSweepFlag();

  /**
   * Tell Peano to run through mesh with same action sets again
   *
   * This flag tells Swift's main function that the step has not been
   * complete, i.e. it has to rerun the same step again once this mesh
   * traversal has completed. Therefore, the name is
   * setRerunPreviousGridSweep() - by the time this information is
   * evaluated it will be ***after*** the mesh sweep.
   *
   * The flag is not automatically reset at any point. You have to do this
   * manually by calling clearRerunPreviousGridSweepFlag(). Consult the
   * documentation there.
   *
   * The flag is automatically reduced by allReduce(), i.e. after the
   * mesh traversal, all ranks will know if one rank has requested a rerun
   * for the species.
   */
  void setRerunPreviousGridSweep();

  /**
   * Set both min and max time stamp.
   */
  void setTimeStamp(double t, bool reduceInsteadOfOverwrite = true);
  void setTimeStamp(double tMin, double tMax, bool reduceInsteadOfOverwrite = true);
  void setTimeStepSize(double dt, bool reduceInsteadOfOverwrite = true);
  void setTimeStepSize(double dtMin, double dtMax, bool reduceInsteadOfOverwrite = true);

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

  void clearSearchRadius();
  void clearVelocity();
  void clearTimeStampAndTimeStepSize();

  void allReduce();
};
