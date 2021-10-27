#include "TimeStepping.h"

#include "tarch/Assertions.h"
#include "tarch/la/la.h"



std::pair<double,double> exahype2::getInterpolationWeights( double oldTimeStampOnFace, double newTimeStampOnFace, double cellTimeStamp ) {
  static tarch::logging::Log _log( "exahype2" );

  double timeSpan = newTimeStampOnFace - oldTimeStampOnFace;
  assertion4( tarch::la::greaterEquals(timeSpan,0.0), timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );
  if ( tarch::la::equals(timeSpan,0.0) ) {
    logDebug( "getInterpolationWeights(double,double,double)", "time span equals zero" );
    return std::pair<double,double>(0.0,1.0);
  }
  else {
    double newWeight = (cellTimeStamp - oldTimeStampOnFace) / timeSpan;

    assertion5( tarch::la::greaterEquals(newWeight,0.0), newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );
    assertion5( tarch::la::smallerEquals(newWeight,1.0), newWeight, timeSpan, oldTimeStampOnFace, newTimeStampOnFace, cellTimeStamp );

    logDebug( "getInterpolationWeights(double,double,double)", "timeSpan" << timeSpan );

    return std::pair<double,double>(1.0-newWeight,newWeight);
  }
}


double exahype2::discretiseAndTruncateTimeStepSizes(
  double cellTimeStepSize,
  double minGlobalTimeStepSize,
  double maxGlobalTimeStepSize,
  double discretisation
) {
  cellTimeStepSize = std::min(cellTimeStepSize,maxGlobalTimeStepSize*1.1);
  if ( tarch::la::smallerEquals(discretisation,1.0) or tarch::la::smallerEquals(minGlobalTimeStepSize,0.0) ) {
    return cellTimeStepSize;
  }
  else if ( tarch::la::smallerEquals(cellTimeStepSize, minGlobalTimeStepSize) ) {
    return cellTimeStepSize;
  }
  else {
    double result = minGlobalTimeStepSize;
    while ( tarch::la::smallerEquals(result,cellTimeStepSize) ) {
      result *= discretisation;
    }
    result /= discretisation;
    return result;
  }
}
