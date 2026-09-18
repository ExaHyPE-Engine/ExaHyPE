// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeStepping.h"

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/ScalarOperations.h"

/**
 * Returns the weights of the old and the new face snapshot for cellTimeStamp. A face whose two
 * time stamps are equal weights the new snapshot with one.
 */
std::pair<double, double> exahype2::getInterpolationWeights(
  double oldTimeStampOnFace,
  double newTimeStampOnFace,
  double cellTimeStamp
) {
  const double timeSpan = newTimeStampOnFace - oldTimeStampOnFace;

  [[maybe_unused]] const bool timeSpanIsNonNegative = tarch::la::greaterEquals(timeSpan, 0.0);
  assertion(timeSpanIsNonNegative, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp);
  if (tarch::la::equals(timeSpan, 0.0)) {
    logDebug("The time span between the two face snapshots equals zero");
    return std::pair<double, double>(0.0, 1.0);
  } else {
    const double newWeight = (cellTimeStamp - oldTimeStampOnFace) / timeSpan;

    [[maybe_unused]] const bool newWeightIsNonNegative = tarch::la::greaterEquals(newWeight, 0.0);
    [[maybe_unused]] const bool newWeightIsAtMostOne   = tarch::la::smallerEquals(newWeight, 1.0);
    assertion(newWeightIsNonNegative, newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp);
    assertion(newWeightIsAtMostOne, newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp);

    logDebug("The time span between the two face snapshots is " << timeSpan);

    return std::pair<double, double>(1.0 - newWeight, newWeight);
  }
}

/**
 * Returns cellTimeStepSize truncated to the subdivision factor times maxGlobalTimeStepSize and
 * bucketed onto the powers of the subdivision factor below the largest time step size seen so far.
 *
 * @param cellTimeStepSize Time step size the patch would like to pick.
 * @param maxGlobalTimeStepSize Maximum global time step size of the previous time step.
 * @param discretisationStepsSize Zero or a negative value switches the bucketing off. A value
 *   above one splits each bucket into that many equal increments.
 */
double exahype2::discretiseAndTruncateTimeStepSizes(
  double cellTimeStepSize,
  double maxGlobalTimeStepSize,
  int    discretisationStepsSize
) {
  // A cell one level coarser than another has a time step larger by the
  // subdivision factor, so the discrete time step sizes form a geometric
  // sequence with that ratio.
  constexpr double TimeStepRatio = peano4::sfc::ActiveCurve::SubdivisionFactor;
  if (tarch::la::greater(maxGlobalTimeStepSize, 0.0) and cellTimeStepSize > maxGlobalTimeStepSize * TimeStepRatio) {
    logDebug(
      "The cell time step size " << cellTimeStepSize << " exceeds the maximum global time step size "
                                 << maxGlobalTimeStepSize << " times the subdivision factor"
    );
    cellTimeStepSize = maxGlobalTimeStepSize * TimeStepRatio;
  }

  if (discretisationStepsSize <= 0) {
    return cellTimeStepSize;
  }

  [[maybe_unused]] const bool cellTimeStepSizeIsPositive = tarch::la::greater(cellTimeStepSize, 0.0);
  assertion(cellTimeStepSizeIsPositive, cellTimeStepSize, maxGlobalTimeStepSize, discretisationStepsSize);

  // Every thread of a traversal calls this function, so a compare-exchange writes the largest
  // time step size seen so far and no update is lost.
  static std::atomic<double> largestTimeStepSize{-1.0};
  static_assert(std::atomic<double>::is_always_lock_free, "A traversal updates this value without a lock.");

  double maxCurrentTimeStepSize = largestTimeStepSize.load(std::memory_order_relaxed);
  while (true) {
    double updatedTimeStepSize = maxCurrentTimeStepSize;
    if (maxCurrentTimeStepSize <= 0.0) {
      updatedTimeStepSize = cellTimeStepSize;
    } else if (maxGlobalTimeStepSize > maxCurrentTimeStepSize) {
      updatedTimeStepSize = maxCurrentTimeStepSize * TimeStepRatio;
    }

    if (updatedTimeStepSize == maxCurrentTimeStepSize) {
      break;
    }

    // A failed exchange writes the value another thread stored into maxCurrentTimeStepSize.
    if (largestTimeStepSize
          .compare_exchange_weak(maxCurrentTimeStepSize, updatedTimeStepSize, std::memory_order_relaxed)) {
      maxCurrentTimeStepSize = updatedTimeStepSize;
      break;
    }
  }

  double result = maxCurrentTimeStepSize;
  while (result > cellTimeStepSize) {
    result /= TimeStepRatio;
  }

  if (discretisationStepsSize > 1) {
    const double incrementOnThisLevel = result / discretisationStepsSize;

    do {
      result += incrementOnThisLevel;
    } while (result < cellTimeStepSize);
    result -= incrementOnThisLevel;
  }

  logDebug("The cell time step size " << cellTimeStepSize << " reduces to " << result);
  return result;
}
