// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_TIME_STEPPING_H_
#define _EXAHYPE2_TIME_STEPPING_H_


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
   * @see exahype2.solvers.fv.kernels.create_postprocess_updated_patch_for_local_time_stepping
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
      if ( tarch::la::greater(leftNeighbourTimeStamp,cellLabel.getTimeStamp()) ) {
        result     = std::min(result,leftNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if ( tarch::la::smaller(leftNeighbourTimeStamp,cellLabel.getTimeStamp()) ) {
         oneIsBehind = true;
      }

      double rightNeighbourTimeStamp = faceLabelEnumerator(d=Dimensions).getNewTimeStamp(1);
      if ( tarch::la::greater(rightNeighbourTimeStamp,cellLabel.getTimeStamp()) ) {
        result     = std::min(result,rightNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if ( tarch::la::smaller(rightNeighbourTimeStamp,cellLabel.getTimeStamp()) ) {
       oneIsBehind = true;
       }
    }

    if (oneIsAhead and not oneIsBehind) {
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
   * @param cellLabel
   */
  template <typename CellLabel, typename FaceLabel>
  bool runTimeStepOnCell(
    const CellLabel& cellLabel,
    const peano4::datamanagement::FaceEnumerator< FaceLabel >& faceLabelEnumerator
  ) {
    double cellTimeStamp =  cellLabel.getTimeStamp();

    return tarch::la::greaterEquals( ::exahype2::internal::getMinTimeStampOfNeighbours(faceLabelEnumerator), cellTimeStamp );
  }


  /**
   * Remove accumulation errors from cell time stamps
   *
   * Use this routine after you have updated the cell's time stamp. If we add
   * time step sizes to the time stamp over and over again, it is very likely
   * that the time stamps at resolution boundaries do not match at one point.
   * If we use subcycling, 3xdt/3 usually does not equal the dt of a big cell.
   *
   * So what I do first of all is to identify if the current (update) cell
   * time stamp does equal a neighbour. Then, I determine the maximum over
   * those that are equal.
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

    double       maxOfMatchingTimeStamps = -1.0;
    const double currentFutureTimeStamp  = cellLabel.getTimeStamp() + timeStepSize;
    for (int d=0; d<Dimensions; d++) {
      if (not faceLabelEnumerator(d).getBoundary() and tarch::la::equals( faceLabelEnumerator(d).getNewTimeStamp(0), currentFutureTimeStamp)) {
        maxOfMatchingTimeStamps = std::max(maxOfMatchingTimeStamps,faceLabelEnumerator(d).getNewTimeStamp(0));
      }
      if (not faceLabelEnumerator(d).getBoundary() and tarch::la::equals( faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1), currentFutureTimeStamp)) {
        maxOfMatchingTimeStamps = std::max(maxOfMatchingTimeStamps,faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1));
      }
    }

    if ( maxOfMatchingTimeStamps>0.0 ) {
      double tweakedTimeStepSize = maxOfMatchingTimeStamps - cellLabel.getTimeStamp();
      assertion( tweakedTimeStepSize>=timeStepSize );
      if (tweakedTimeStepSize>timeStepSize) {
        logDebug( "removeTimeStepAccummulationErrorsFromCell(...)", "adopt local time step from " << timeStepSize << " to " << tweakedTimeStepSize );
      }
      return tweakedTimeStepSize;
    }
    else return timeStepSize;
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
   * Discretise (bucket) time step sizes
   *
   * We expect a min time step size that we use globally. We find the biggest
   * @f$ discretisationSteps^k \cdot minGlobalTimeStepSize < cellTimeStepSize @f$
   * value through k which still meets the stability of cellTimeStepSize. We
   * then return this value.
   *
   * @param discretisationSteps Pass in zero or something negative to switch
   *   discretisation off.
   */
  double discretiseTimeStepSizes(
    double cellTimeStepSize,
    double minGlobalTimeStepSize,
    double discretisationSteps
  );
}

#endif
