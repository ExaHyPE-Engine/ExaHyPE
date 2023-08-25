#pragma once


#include <vector>

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace applications {
  namespace exahype2 {
    namespace euler {
      namespace sphericalaccretion {
        class MassAccumulator;
      } // namespace sphericalaccretion
    }   // namespace euler
  }     // namespace exahype2
} // namespace applications


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
 * @image html onion-model.png
 *
 * ## Interpolation
 *
 *
 * @author Han Zhang, Tobias Weinzierl
 */
class applications::exahype2::euler::sphericalaccretion::MassAccumulator {
private:
  static tarch::logging::Log _log;

  tarch::multicore::BooleanSemaphore _semaphore;

  const double _shellWidth;
  const double _threshold;

  /**
   * Holds mass per shell. Is used to accumulate data.
   */
  std::vector<double> _currentMass;

  /**
   * Holds accumulated mass per shell.
   */
  std::vector<double> _previousMassAccumulated;

  double _maxDistanceThatMakesContribution;

  /**
   * Identifies the right bucket. There's no guarantee that such a bucket
   * does exist already
   */
  int getBucket(double radius) const;

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
   * Finalise data accumulation for next time step
   *
   * The routine runs through a sequence of steps:
   *
   * - Allreduce _currentMass such that all data from all ranks is available
   *   on all ranks. The reduction operator is the sum.
   * - Roll over the current data set into previousMassAccumulated and clear
   *   that latter. It is, from now on, available for the next accumulation
   *   again.
   * - Compute accumulated data by running through shells inside-out.
   *   _currentMass holds the mass per shell, but eventually we are
   *   interested in the accumulated mass, i.e. the integral from the center
   *   over the radius r. Note that some shells can be empty as either no
   *   (over-)mass is there anymore or a shell was not hit by an integration
   *   point or finite volume centre. So the accumulated result is
   *   monotonously growing (usually), but the growth is not strict
   *   monotonously growing.
   * - Find out from which radius on the accumulated data makes no big
   *   difference anymore. Truncate the data series here. We effectively now
   *   know the maximum radius which makes a difference to the mass
   *   accumulation.
   *
   *
   * ## Cut off radius
   *
   * For the cut-off radius, I run through all differences between successive
   * accumulated shells/buckets. If two successive buckets have a (relative)
   * difference which is bigger than 1.0+_threshold, i.e. they differ
   * significantly, this is a potential cut-off: the bigger bucket is one we
   * should definitely still take into account. I take the maximum over all
   * of these cut-off candidates.
   *
   * It is important that we analyse all bucket differences: We cannot stop
   * this analysis early, as some shells might be empty (see reasons above).
   * So even if two buckets i and i+1 do not differ at all, it might still be
   * that i+1 and i+2 differ significantly.
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
