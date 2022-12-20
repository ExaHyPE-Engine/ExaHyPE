// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/ScalarOperations.h"
#include "tarch/logging/Log.h"

#include "peano4/datamanagement/FaceEnumerator.h"

#include <utility>


namespace exahype2 {
  namespace internal {
    /**
     * Run over all neighbours and analyse their time stamp.
     *
     */
    template <typename FaceLabel>
    double getMinTimeStampOfNeighbours(
      const peano4::datamanagement::FaceEnumerator< FaceLabel >& faceLabelEnumerator
    ) {
      double result = std::numeric_limits<double>::max();

      for (int d=0; d<Dimensions; d++) {
        result = std::min(result, faceLabelEnumerator(d).getNewTimeStamp(0) );
        result = std::min(result, faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1) );
      }

      return result;
    }
  }

  /**
   * Similar to getMinTimeStampOfNeighbours(), but we minimise only over those
   * neighbours that are actually ahead. If no neighbour is ahead or one lags
   * behind, we return the time stamp of cellLabel.
   *
   * ## Using code/algorithms
   *
   * Local timestepping codes can run into situations where the eigenvalue per
   * cell is zero. This means that nothing happens in such cell. Therefore, the
   * cell cannot advance in time. We don't know how far we can/should march. In
   * this case, ExaHyPE offers two strategies: We can use the global admissible
   * time step size, i.e. the global maximum eigenvalue, to determine a time
   * step size; or we can see whether one of the neighbours is ahead and catch
   * up.
   *
   * The former method leads to a staircase pattern, as some cells where
   * nothing happens race forward in time. A description of this behaviour is
   * found in the Python routine referenced below.
   *
   * ## Deadlocks
   *
   * I've seen multiple cases where
   *
   * @see exahype2.solvers.fv.kernels.create_compute_new_time_step_size_kernel_for_local_time_stepping
   */
  template <typename CellLabel, typename FaceLabel>
  double getMinTimeStampOfNeighboursAhead(
    const CellLabel& cellLabel,
    const peano4::datamanagement::FaceEnumerator< FaceLabel >& faceLabelEnumerator
  ) {
    double result = std::numeric_limits<double>::max();
    bool   oneIsAhead  = false;
    bool   oneIsBehind = false;

    for (int d=0; d<Dimensions; d++) {
      double leftNeighbourTimeStamp = faceLabelEnumerator(d).getNewTimeStamp(0);
      if (
        not faceLabelEnumerator(d).getBoundary()
        and
        tarch::la::greater(leftNeighbourTimeStamp,cellLabel.getTimeStamp())
      ) {
        result     = std::min(result,leftNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if (
        not faceLabelEnumerator(d).getBoundary()
        and
        tarch::la::smaller(leftNeighbourTimeStamp,cellLabel.getTimeStamp())
      ) {
        oneIsBehind = true;
      }

      double rightNeighbourTimeStamp = faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1);
      if (
        not faceLabelEnumerator(d+Dimensions).getBoundary()
        and
        tarch::la::greater(rightNeighbourTimeStamp,cellLabel.getTimeStamp())
      ) {
        result     = std::min(result,rightNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if (
        not faceLabelEnumerator(d+Dimensions).getBoundary()
        and
        tarch::la::smaller(rightNeighbourTimeStamp,cellLabel.getTimeStamp())
      ) {
        oneIsBehind = true;
      }
    }

    if (oneIsAhead and not oneIsBehind) {
      assertion(result>cellLabel.getTimeStamp());
      return result;
    }
    else {
      return cellLabel.getTimeStamp();
    }
  }

  /**
   * Determine whether to run a time step on a cell by analysing the
   * neighbouring cells' timestamp. These timestamps are stored within the
   * face labels. We update a cell if all the neighbours are at the same
   * time stamp or ahead.
   *
   * @see removeTimeStepAccumulationErrorsFromCell() for a discussion of
   *   precisions we use for the checks above
   *
   * For some reason that I haven't understood completely yet, the above
   * decision pattern can still lead to a deadlock where no cell in the
   * global domain does advance in time. Which is really weird, as there
   * always should be a global minimum. My guess is that something along
   * the AMR boundaries does not work properly.
   *
   * @param cellLabel
   */
  template <typename CellLabel, typename FaceLabel>
  bool runTimeStepOnCell(
    const CellLabel&                                            cellLabel,
    const peano4::datamanagement::FaceEnumerator< FaceLabel >&  faceLabelEnumerator,
    double                                                      globalMinTimeStamp
  ) {
    double cellTimeStamp    = cellLabel.getTimeStamp();
    double cellTimeStepSize = cellLabel.getTimeStepSize();

    const double Tolerance = 0.01;

    bool result = tarch::la::greaterEquals( ::exahype2::internal::getMinTimeStampOfNeighbours(faceLabelEnumerator), cellTimeStamp, Tolerance*cellTimeStepSize )
               or tarch::la::equals(cellTimeStamp, globalMinTimeStamp);

    // @todo das ist net das Problem. Das Problem ist, dass ich u.U. keine Eigenwerte habe
    if (not result) {
//      std::cout << "skip " << cellLabel.toString() << " as neighbours have t_min=" << ::exahype2::internal::getMinTimeStampOfNeighbours(faceLabelEnumerator) << std::endl;
    }
    return result;
  }


  /**
   * Remove accumulation errors from cell time stamps
   *
   * If we add
   * time step sizes to the time stamp over and over again, it is very likely
   * that the time stamps at resolution boundaries do not match at one point.
   * If we use subcycling, 3xdt/3 usually does not equal the dt of a big cell.
   * So it makes sense to amend the time step sizes slightly to help us to
   * synchronise time stamps between cells, i.e. to avoid that they are off by
   * a tiny bit and hence mess up the time stepping.
   *
   * So what I do first of all is to check if the current (update) cell
   * time stamp does equal a neighbour. If there are multiple of matching
   * neighbours, I pick the one with the biggest time stamp. This check for
   * equality is a double precision check which should use relative quantities,
   * as time stamps can be arbitrarily large. So once I find a matching time
   * stamp, I return this time stamp rather than the current time step size
   * and, hence, sync the current time step size with the neighbours. This can
   * mean that I reduce or increase the time step size slightly, i.e. I assume
   * that enough slack is built into the chosen time step size such that a tiny
   * increase does not blow up the result.
   *
   * ## Relative accuracy
   *
   * If two time stamps are equal depends on the magnutide of the time step
   * size. It does not matter what the time stamps are, as any "rounding"
   * decision should be time translation invariant. However, if I have a
   * large time step size, then I'd be happy to alter it more rigorously than
   * for a stiff problem with tiny time step sizes. Overall, I accept around one
   * percent of the time step sizes as acceptable alteration of the outcome.
   *
   * ## Usage of routine within local time stepping
   *
   * If we have subcycling, then we know that timeStepSize is never zero, as we use
   * the largest (original) eigenvalue within the system and derive the levels'
   * time step sizes from there. Things are different with local time stepping, where
   * each cell can have its own time step size. If a cell has a size of zero, then
   * we always should derive the admissible dt from the neighbours.
   *
   * @see getMinTimeStampOfNeighboursAhead() for a discussion how to make zero
   *   eigenvalue patches catch up over time.
   *
   *
   * @param timeStepSize Time step size the code would wish to use
   * @return Time step size the code should use eventually
   */
  template <typename CellLabel, typename FaceLabel>
  double removeTimeStepAccumulationErrorsFromCell(
    const CellLabel&  cellLabel,
    const peano4::datamanagement::FaceEnumerator< FaceLabel >&  faceLabelEnumerator,
    double  timeStepSize
  ) {
    static tarch::logging::Log _log( "exahype2" );

    const double Tolerance = 0.01;
    double relativeTolerance = tarch::la::equals(timeStepSize,0.0) ? tarch::la::NUMERICAL_ZERO_DIFFERENCE : Tolerance * timeStepSize;

    double       maxOfMatchingTimeStamps = -1.0;
    const double currentFutureTimeStamp  = cellLabel.getTimeStamp() + timeStepSize;
    for (int d=0; d<Dimensions; d++) {
      if (
        not faceLabelEnumerator(d).getBoundary()
        and
        tarch::la::equals( faceLabelEnumerator(d).getNewTimeStamp(0), currentFutureTimeStamp, relativeTolerance)
      ) {
        maxOfMatchingTimeStamps = std::max(maxOfMatchingTimeStamps,faceLabelEnumerator(d).getNewTimeStamp(0));
      }
      if (
        not faceLabelEnumerator(d).getBoundary()
        and
        tarch::la::equals( faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1), currentFutureTimeStamp, relativeTolerance)
      ) {
        maxOfMatchingTimeStamps = std::max(maxOfMatchingTimeStamps,faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1));
      }
    }

    if ( maxOfMatchingTimeStamps>0.0 ) {
      double tweakedTimeStepSize = maxOfMatchingTimeStamps - cellLabel.getTimeStamp();
      assertion2( tarch::la::greaterEquals( tweakedTimeStepSize,timeStepSize ), tweakedTimeStepSize, timeStepSize );
      if (tweakedTimeStepSize>timeStepSize) {
        logDebug( "removeTimeStepAccummulationErrorsFromCell(...)", "adopt local time step from " << timeStepSize << " to " << tweakedTimeStepSize );
      }
      timeStepSize = tweakedTimeStepSize;
    }

    logDebug( "removeTimeStepAccummulationErrorsFromCell(...)", "dt=" << timeStepSize << ", max-match=" << maxOfMatchingTimeStamps );
    return timeStepSize;
  }


  /**
   *
   * <h2> A posteriori fixes of time stamps </h2>
   *
   * I'm afraid of accumulation errors in the time stamps for the small cells
   * which might end up lacking behind or running ahead. I can recognise such
   * situations by very small weights for the old time snapshot. However, I
   * don't fix it here. For fixing such a divergence,  I rely on an explicit
   * cleanup later on.
   *
   *
   * @param cellTimeStamp This is an in-out parameter, as we can adopt the stamp size to
   *   avoid the accummulation of rounding errors.
   * @return Weight of (old,new) data.
   */
  std::pair<double,double> getInterpolationWeights( double oldTimeStampOnFace, double newTimeStampOnFace, double cellTimeStamp );

  /**
   * Discretise (bucket) time step sizes and truncate it
   *
   * This routine is used by any local time stepping. The idea is that we first
   * mimick subcycling: Let there be a global max time step size. We assume this
   * one arises on the coarsest mesh level. So patches either do this time step
   * or they do a time step that is @f$ 3^{-k} @f$ that size.
   *
   * ## Pitfalls
   *
   *
   *
   *
   * We expect a min time step size that we use globally. We find the biggest
   * @f$ discretisationSteps^k \cdot minGlobalTimeStepSize < cellTimeStepSize @f$
   * value through k which still meets the stability of cellTimeStepSize. We
   * then return this value.
   *
   * ## Truncation
   *
   * If the eigenvalues become very small within a cell, we end up with huge
   * time step sizes. This should not happen. So I expect the global time step
   * size (largest value) and truncate the time step size in any case by this one.
   *
   * ## Decreasing time step sizes
   *
   * I use the global minimal time step size to kick off the analysis. This fails
   * if the admissible global time step size shrinks over time. Therefore, the
   * cell's time step size can be smaller than the globally admissible time step
   * size. It simply means that the global time step size is shrinking and that
   * the argument we get is lagging behind as we haven't finished the current
   * time step yet.
   *
   *
   *
   * @param discretisationSteps Pass in zero to allow a totally anarchic time
   *   stepping. Use one to construct subcycling without a re-adoption
   *   of the time step sizes or something negative to switch the bucketing off.
   *
   * @param maxGlobalTimeStepSize Maximum global time step size (of previous
   *   time step). I use this one to truncate too big time step sizes. Overall,
   *   I expect the time step size not to grow by more than 10 percent.
   *
   * @param cellTimeStepSize Time step size the patch would like to pick.
   */
  double discretiseAndTruncateTimeStepSizes(
    double cellTimeStepSize,
    double maxGlobalTimeStepSize,
    int    discretisationStepsSize
  );
}

