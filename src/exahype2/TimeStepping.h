// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/FaceEnumerator.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/ScalarOperations.h"

namespace exahype2 {
  namespace internal {
    /**
     * Returns the smallest new time stamp stored on the faces of one cell.
     */
    template <typename FaceLabel>
    double getMinTimeStampOfNeighbours(const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator) {
      double result = std::numeric_limits<double>::max();

      for (int d = 0; d < DIMENSIONS; d++) {
        result = std::min(result, faceLabelEnumerator(d).getNewTimeStamp(0));
        result = std::min(result, faceLabelEnumerator(d + DIMENSIONS).getNewTimeStamp(1));
      }

      return result;
    }
  } // namespace internal

  /**
   * Returns the smallest time stamp over the neighbours that are ahead of cellLabel. If no
   * neighbour is ahead or one lags behind, the routine returns the time stamp of cellLabel.
   */
  template <typename CellLabel, typename FaceLabel>
  double getMinTimeStampOfNeighboursAhead(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator
  ) {
    double result      = std::numeric_limits<double>::max();
    bool   oneIsAhead  = false;
    bool   oneIsBehind = false;

    for (int d = 0; d < DIMENSIONS; d++) {
      const double leftNeighbourTimeStamp = faceLabelEnumerator(d).getNewTimeStamp(0);
      if (not faceLabelEnumerator(d).getBoundary()
          and tarch::la::greater(leftNeighbourTimeStamp, cellLabel.getTimeStamp())) {
        result     = std::min(result, leftNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if (not faceLabelEnumerator(d).getBoundary()
          and tarch::la::smaller(leftNeighbourTimeStamp, cellLabel.getTimeStamp())) {
        oneIsBehind = true;
      }

      const double rightNeighbourTimeStamp = faceLabelEnumerator(d + DIMENSIONS).getNewTimeStamp(1);
      if (not faceLabelEnumerator(d + DIMENSIONS).getBoundary()
          and tarch::la::greater(rightNeighbourTimeStamp, cellLabel.getTimeStamp())) {
        result     = std::min(result, rightNeighbourTimeStamp);
        oneIsAhead = true;
      }
      if (not faceLabelEnumerator(d + DIMENSIONS).getBoundary()
          and tarch::la::smaller(rightNeighbourTimeStamp, cellLabel.getTimeStamp())) {
        oneIsBehind = true;
      }
    }

    if (oneIsAhead and not oneIsBehind) {
      assertion(result > cellLabel.getTimeStamp());
      return result;
    } else {
      return cellLabel.getTimeStamp();
    }
  }

  /**
   * Returns whether the cell may run a time step. The cell advances if every neighbour carries
   * the same time stamp or is ahead, or if the cell sits at the global minimum time stamp.
   */
  template <typename CellLabel, typename FaceLabel>
  bool runTimeStepOnCell(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator,
    double                                             globalMinTimeStamp
  ) {
    const double cellTimeStamp    = cellLabel.getTimeStamp();
    const double cellTimeStepSize = cellLabel.getTimeStepSize();

    const double Tolerance = 0.01;

    const bool result = tarch::la::greaterEquals(
                          ::exahype2::internal::getMinTimeStampOfNeighbours(faceLabelEnumerator),
                          cellTimeStamp,
                          Tolerance * cellTimeStepSize
                        )
      or tarch::la::equals(cellTimeStamp, globalMinTimeStamp);

    return result;
  }

  /**
   * Returns the largest neighbour time stamp that matches the cell's future time stamp within
   * the relative tolerance, minus the cell's time stamp, and timeStepSize when no neighbour
   * matches. Adding time step sizes over and over lets the time stamps at resolution boundaries
   * drift apart, and the match removes that drift.
   *
   * @param timeStepSize Time step size the code would wish to use.
   * @return Time step size the code should use eventually.
   */
  template <typename CellLabel, typename FaceLabel>
  double removeTimeStepAccumulationErrorsFromCell(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator,
    double                                             timeStepSize
  ) {
    const double Tolerance = 0.01;
    const double relativeTolerance =
      tarch::la::equals(timeStepSize, 0.0) ? tarch::la::NUMERICAL_ZERO_DIFFERENCE : Tolerance * timeStepSize;

    double       maxOfMatchingTimeStamps = -1.0;
    const double currentFutureTimeStamp  = cellLabel.getTimeStamp() + timeStepSize;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (not faceLabelEnumerator(d).getBoundary()
          and tarch::la::equals(faceLabelEnumerator(d).getNewTimeStamp(0), currentFutureTimeStamp, relativeTolerance)) {
        maxOfMatchingTimeStamps = std::max(maxOfMatchingTimeStamps, faceLabelEnumerator(d).getNewTimeStamp(0));
      }
      if (not faceLabelEnumerator(d).getBoundary()
          and tarch::la::equals(
            faceLabelEnumerator(d + DIMENSIONS).getNewTimeStamp(1),
            currentFutureTimeStamp,
            relativeTolerance
          )) {
        maxOfMatchingTimeStamps =
          std::max(maxOfMatchingTimeStamps, faceLabelEnumerator(d + DIMENSIONS).getNewTimeStamp(1));
      }
    }

    if (maxOfMatchingTimeStamps > 0.0) {
      const double tweakedTimeStepSize = maxOfMatchingTimeStamps - cellLabel.getTimeStamp();
      assertion(tarch::la::greaterEquals(tweakedTimeStepSize, timeStepSize), tweakedTimeStepSize, timeStepSize);
      if (tweakedTimeStepSize > timeStepSize) {
        logDebug("Adopt local time step from " << timeStepSize << " to " << tweakedTimeStepSize);
      }
      timeStepSize = tweakedTimeStepSize;
    }

    logDebug("dt=" << timeStepSize << ", max-match=" << maxOfMatchingTimeStamps);
    return timeStepSize;
  }

  std::pair<double, double> getInterpolationWeights(
    double oldTimeStampOnFace,
    double newTimeStampOnFace,
    double cellTimeStamp
  );

  double discretiseAndTruncateTimeStepSizes(
    double cellTimeStepSize,
    double maxGlobalTimeStepSize,
    int    discretisationStepsSize
  );
} // namespace exahype2
