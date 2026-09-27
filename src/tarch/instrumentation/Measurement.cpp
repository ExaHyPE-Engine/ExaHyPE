// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Measurement.h"

#include "tarch/Assertions.h"

double
tarch::instrumentation::Measurement::getAccumulatedValue() const {
  return _accumulatedValue;
}

/** Returns the sum of the values divided by their count, and 0 before the first setValue() call. */
double
tarch::instrumentation::Measurement::getAverageValue() const {
  if (_numberOfMeasurements == 0) {
    assertion((_accumulatedValue) == (0.0), _accumulatedValue, 0.0);
    return 0.0;
  } else {
    return _accumulatedValue / static_cast<double>(_numberOfMeasurements);
  }
}

void
tarch::instrumentation::Measurement::setValue(const double& value) {
  _accumulatedValue += value;
  _numberOfMeasurements += 1;
}

int
tarch::instrumentation::Measurement::getNumberOfMeasurements() const {
  return _numberOfMeasurements;
}
