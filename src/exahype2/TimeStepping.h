// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_TIME_STEPPING_H_
#define _EXAHYPE2_TIME_STEPPING_H_


#include "tarch/la/ScalarOperations.h"
#include "tarch/logging/Log.h"

#include "peano4/datamanagement/FaceEnumerator.h"

#include <utility>


namespace exahype2 {
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
    static tarch::logging::Log _log( "exahype2" );

    double cellTimeStamp =  cellLabel.getTimeStamp();

    bool updateCell = true;

    for (int d=0; d<Dimensions; d++) {
      updateCell &= tarch::la::greaterEquals(
        faceLabelEnumerator(d).getNewTimeStamp(0),
        cellTimeStamp
      );
      updateCell &= tarch::la::greaterEquals(
        faceLabelEnumerator(d+Dimensions).getNewTimeStamp(1),
        cellTimeStamp
      );
    }

    return updateCell;
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
}

#endif
