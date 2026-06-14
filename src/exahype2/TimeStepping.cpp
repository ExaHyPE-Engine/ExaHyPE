#include "TimeStepping.h"

#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"

std::pair<double,double> exahype2::getInterpolationWeights( double oldTimeStampOnFace, double newTimeStampOnFace, double cellTimeStamp ) {

  double timeSpan = newTimeStampOnFace - oldTimeStampOnFace;
  assertion( tarch::la::greaterEquals(timeSpan,0.0), timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );
  if ( tarch::la::equals(timeSpan,0.0) ) {
    logDebug("time span equals zero");
    return std::pair<double,double>(0.0,1.0);
  }
  else {
    double newWeight = (cellTimeStamp - oldTimeStampOnFace) / timeSpan;

    assertion( tarch::la::greaterEquals(newWeight,0.0), newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );
    assertion( tarch::la::smallerEquals(newWeight,1.0), newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );

    logDebug("timeSpan" << timeSpan);

    return std::pair<double,double>(1.0-newWeight,newWeight);
  }
}

double exahype2::discretiseAndTruncateTimeStepSizes(
  double cellTimeStepSize,
  double maxGlobalTimeStepSize,
  int    discretisationStepsSize
) {

  if (tarch::la::greater(maxGlobalTimeStepSize,0.0) and cellTimeStepSize>maxGlobalTimeStepSize*3.0) {
    logDebug("truncate cell time step size from " << cellTimeStepSize << " due to max global dt=" << maxGlobalTimeStepSize);
    cellTimeStepSize = maxGlobalTimeStepSize*3.0;
  }

  if (discretisationStepsSize<=0) {
    return cellTimeStepSize;
  }

  static double maxCurrentTimeStepSize = -1.0;

  if (maxCurrentTimeStepSize<=0.0) {
    maxCurrentTimeStepSize = cellTimeStepSize;
  }
  else if (maxGlobalTimeStepSize>maxCurrentTimeStepSize) {
    maxCurrentTimeStepSize *= 3.0;
  }

  double result = maxCurrentTimeStepSize;
  while (result > cellTimeStepSize) {
    result /= 3.0;
  }

  if (discretisationStepsSize>1) {
    const double incrementOnThisLevel = result / discretisationStepsSize;

    do {
      result += incrementOnThisLevel;
    } while (result<cellTimeStepSize);
    result -= incrementOnThisLevel;
  }

  logDebug("reduce dt=" << cellTimeStepSize << " to " << result);
  return result;
}
