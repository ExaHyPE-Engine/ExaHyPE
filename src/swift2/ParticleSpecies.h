// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/multicore/Lock.h"

#ifdef Parallel
#include "tarch/mpi/mpi.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/Service.h"
#include "tarch/services/ServiceRepository.h"
#endif

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
  inline ParticleSpecies() : _minTimeStepSize(std::numeric_limits<double>::max()),
    _maxTimeStepSize(0.0),
    _minTimeStamp(0.0), // std::numeric_limits<double>::max()
    _maxTimeStamp(0.0),
    _minSearchRadius(std::numeric_limits<double>::max()),
    _maxSearchRadius(0.0),
    _minVelocity(std::numeric_limits<double>::max()),
    _maxVelocity(0.0),
    _rerunPreviousGridSweep(false) {}

  ParticleSpecies(const ParticleSpecies& copy) = delete;

  inline double getMinTimeStepSize() const {
    return _minTimeStepSize;
  }

  inline double getMaxTimeStepSize() const {
    return _maxTimeStepSize;
  }

  inline double getMinTimeStamp() const {
    return _minTimeStamp;
  }

  inline double getMaxTimeStamp() const {
    return _maxTimeStamp;
  }

  inline double getMinSearchRadius() const {
    return _minSearchRadius;
  }

  inline double getMaxSearchRadius() const {
    return _maxSearchRadius;
  }

  inline double getMinVelocity() const {
    return _minVelocity;
  }

  inline double getMaxVelocity() const {
    return _maxVelocity;
  }

  /**
   * Query species whether the last grid sweep/update sweep has to be
   * repeated. This happens if the search radius changes for example.
   */
  inline bool rerunPreviousGridSweep() const { return _rerunPreviousGridSweep; }

  /**
   * Set the rerun flag back to false
   *
   * This operation has to be called explicitly in your code, i.e. we do not
   * invoke it automatically. A canonical location to call it is the
   * prepare_traversal_kernel event of the corresponding AlgorithmStep of
   * your species.
   */
  inline void clearRerunPreviousGridSweepFlag() { _rerunPreviousGridSweep = false; }

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
  inline void setRerunPreviousGridSweep() {
    tarch::multicore::Lock lock(_semaphore);
    _rerunPreviousGridSweep = true;
  }

  /**
   * Set both min and max time stamp.
   */
  inline void setTimeStamp(double t, bool reduceInsteadOfOverwrite = true) {
    setTimeStamp(t, t, reduceInsteadOfOverwrite);
  }

  inline void setTimeStamp(double tMin, double tMax, bool reduceInsteadOfOverwrite = true) {
    tarch::multicore::Lock lock(_semaphore);
    if (reduceInsteadOfOverwrite) {
      _minTimeStamp = std::min(_minTimeStamp, tMin);
      _maxTimeStamp = std::max(_maxTimeStamp, tMax);
    } else {
      _minTimeStamp = tMin;
      _maxTimeStamp = tMax;
    }
  }

  inline void setTimeStepSize(double dt, bool reduceInsteadOfOverwrite = true) {
    setTimeStepSize(dt, dt, reduceInsteadOfOverwrite);
  }

  inline void setTimeStepSize(double dtMin, double dtMax, bool reduceInsteadOfOverwrite = true) {
    tarch::multicore::Lock lock(_semaphore);
    if (reduceInsteadOfOverwrite) {
      _minTimeStepSize = std::min(_minTimeStepSize, dtMin);
      _maxTimeStepSize = std::max(_maxTimeStepSize, dtMax);
    } else {
      _minTimeStepSize = dtMin;
      _maxTimeStepSize = dtMax;
    }
  }

  /**
   * Set the maximum velocity that has been observed in a cell. This routine
   * takes the maximum velocity as observed so far and updates it if value is
   * bigger.
   *
   * The routine is thread-safe.
   */
  inline void setVelocity(double vMin, double vMax, double rMin, double rMax) {
    tarch::multicore::Lock lock(_semaphore);
    _minVelocity     = std::min(_minVelocity, vMin);
    _maxVelocity     = std::max(_maxVelocity, vMax);
    _minSearchRadius = std::min(_minSearchRadius, rMin);
    _maxSearchRadius = std::max(_maxSearchRadius, rMax);
    logDebug("setVelocity()", vMin << "x" << vMax);
  }

  /**
   * This routine is automatically called for each and every species by the
   * GlobalState object.
   */
  inline void startTimeStep() {}

  /**
   *
   * This routine is automatically called for each and every species by the
   * GlobalState object.
   */
  inline void finishTimeStep() {}

  inline void clearSearchRadius() {
    _minSearchRadius = std::numeric_limits<double>::max();
    _maxSearchRadius = 0.0;
  }

  inline void clearVelocity() {
    _minVelocity = std::numeric_limits<double>::max();
    _maxVelocity = 0.0;
  }

  /**
   * Clear all the time step sizes and time stamps
   *
   * These values then can be reduced again through
   *
   * - setTimeStepSize()
   * - setTimeStamp()
   *
   * before they are actually queried through getMinTimeStepSize() and related
   * routines.
   */
  inline void clearTimeStampAndTimeStepSize() {
    _minTimeStepSize = std::numeric_limits<double>::max();
    _maxTimeStepSize = 0.0;

    _minTimeStamp = std::numeric_limits<double>::max();
    _maxTimeStamp = 0.0;
  }

  inline void allReduce() {
#ifdef Parallel

    double localMinTimeStepSize = _minTimeStepSize;
    double localMaxTimeStepSize = _maxTimeStepSize;

    double localMinTimeStamp = _minTimeStamp;
    double localMaxTimeStamp = _maxTimeStamp;

    double localMinSearchRadius = _minSearchRadius;
    double localMaxSearchRadius = _maxSearchRadius;

    double localMinVelocity = _minVelocity;
    double localMaxVelocity = _maxVelocity;

    int localRerunPreviousGridSweep  = _rerunPreviousGridSweep ? 1 : 0;
    int globalRerunPreviousGridSweep = 0;

    tarch::mpi::Rank::getInstance()
      .allReduce(&localMinTimeStepSize, &_minTimeStepSize, 1, MPI_DOUBLE, MPI_MIN, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });
    tarch::mpi::Rank::getInstance()
      .allReduce(&localMaxTimeStepSize, &_maxTimeStepSize, 1, MPI_DOUBLE, MPI_MAX, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });
    tarch::mpi::Rank::getInstance().allReduce(&localMinTimeStamp, &_minTimeStamp, 1, MPI_DOUBLE, MPI_MIN, [&]() -> void {
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    });
    tarch::mpi::Rank::getInstance().allReduce(&localMaxTimeStamp, &_maxTimeStamp, 1, MPI_DOUBLE, MPI_MAX, [&]() -> void {
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    });
    tarch::mpi::Rank::getInstance()
      .allReduce(&localMinSearchRadius, &_minSearchRadius, 1, MPI_DOUBLE, MPI_MIN, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });
    tarch::mpi::Rank::getInstance()
      .allReduce(&localMaxSearchRadius, &_maxSearchRadius, 1, MPI_DOUBLE, MPI_MAX, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });
    tarch::mpi::Rank::getInstance().allReduce(&localMinVelocity, &_minVelocity, 1, MPI_DOUBLE, MPI_MIN, [&]() -> void {
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    });
    tarch::mpi::Rank::getInstance().allReduce(&localMaxVelocity, &_maxVelocity, 1, MPI_DOUBLE, MPI_MAX, [&]() -> void {
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    });
    tarch::mpi::Rank::getInstance()
      .allReduce(&localRerunPreviousGridSweep, &globalRerunPreviousGridSweep, 1, MPI_INT, MPI_SUM, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });
    _rerunPreviousGridSweep = globalRerunPreviousGridSweep > 0;

#endif
  }
};
