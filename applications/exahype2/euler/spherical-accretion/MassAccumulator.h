#ifndef __APPLICATIONS_EXAHYPE2_EULER_SPHERICAL_ACCRETION_MASS_ACCUMULATOR_H__
#define __APPLICATIONS_EXAHYPE2_EULER_SPHERICAL_ACCRETION_MASS_ACCUMULATOR_H__


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"
#include <vector>


namespace applications {
  namespace exahype2 {
    namespace euler {
      namespace sphericalaccretion {
        class MassAccumulator;
      }
    }
  }
}


/**
 * Simple spherical mass accumulator
 *
 * The idea of this accumulator is that you call it for each volume and the
 * accumulator then, well, accumulates these volumes. For this, it splits the
 * domain around a point into shells, and it keeps track how much mass you
 * find per shell. We discretise the domain into shells, and keep track of some
 * 1d data (mass per shell). Most applications don't use the bookkeeping for
 * the actual mass, but only track the mass resulting from overdensities.
 *
 * Internally, the accumulator holds two copies of the mass: the current one
 * into which we accumulate, and the data from the previous accumulation. This
 * way, we can roll over the data after a time step and thus effectively reset
 * the accumulation, without loosing the original data.
 *
 *
 * ## Thread safety and MPI
 *
 * The class is thread safe. Every rank should have one instance of this class.
 * At the end of each time step, the instances synchronise automatically.
 *
 * ## Bucketing
 *
 * We split the area around a point into shells of fixed size. The number of
 * shells is not fixed a priori, i.e. in theory it could grow as the buckets
 * are built up upon demand.
 *
 *
 * ## Interpolation
 *
 *
 * @author Han Zhang, Tobias Weinzierl
 */
class applications::exahype2::euler::sphericalaccretion::MassAccumulator {
  private:
    static tarch::logging::Log          _log;

    tarch::multicore::BooleanSemaphore  _semaphore;

    const double _shellWidth;
    const double _threshold;

    std::vector<double>  _currentMass;
    std::vector<double>  _previousMassAccumulated;

    double               _maxDistanceThatMakesContribution;

    /**
     * Identifies the right bucket. There's no guarantee that such a bucket
     * does exist already
     */
    int getBucket( double radius ) const;
  public:
    /**
     * @param shellWidth Defines the width of the individual shells
     * @param threshold  After a time step, we accumulate the shells from inside to
     *                   outside. If an additional outer shell's relative
     *                   contribution to the total mass so far is smaller than
     *                   threshold, we throw all further shells away.
     */
    MassAccumulator(double shellWidth = 0.01, double threshold = 0.01);
    void addMass(double mass, double radius);

    /**
     * If you try to read the mass for a distance where we had no accumulation yet,
     * then this routine returns 0. The routine therefore is safe to use even in
     * the first time step, where _previousMass is definitely empty.
     *
     * @param radius
     */
    double getMass_piecewiseConstantInterpolation(double radius) const;
    double getMass_linearInterpolation(double radius) const;

    /**
     * Roll over the current (accumulating) data set into previous mass, determine
     * new cut-off, i.e. biggest shell that will play a role, the total mass within
     * those shells that count, and write some info.
     *
     * Call this routine after each time step, not after each grid sweep.
     */
    void finishAccumulation();

    /**
     * Return the biggest radius which contributes towards mass
     *
     * We know after each step that only a finite number of shells contain meaningful
     * data. The number of shells that we maintain is bounded. We furthermore assume
     * that the solution does not change suddenly, i.e. the mass distribution evolves
     * smoothly in time. This makes us assume that the number of relevant shells grows
     * at most by one per time step, which in turn defines the maximum radius which
     * actually makes a contribution to the total mass.
     *
     * @return Max radius which we think might yield meaningful data
     */
    double getMaxRelevantRadius() const;

    /**
     * This routine returns the total mass. This is the mass accumulated in all
     * shells, i.e. the accumulated mass of the biggest shell.
     *
     * @return Total mass accumulated
     */
    double getTotalMass() const;
};



#endif
