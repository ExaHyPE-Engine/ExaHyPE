// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Measurement.h"

#include "Log.h"
#include "tarch/Assertions.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

tarch::instrumentation::Measurement::Measurement(const double& accuracy):
  _accuracy(accuracy),
  _accumulatedValue(0.0),
  _lastValue(0.0),
  _accumulatedSquares(0.0),
  _numberOfMeasurements(0),
  _isAccurateValue(false),
  _min(std::numeric_limits<double>::max()),
  _max(-std::numeric_limits<double>::max()),
  _minMeasurement(-1),
  _maxMeasurement(-1) {

  assertion1(accuracy >= 0.0, accuracy);
}

void tarch::instrumentation::Measurement::erase() {
  _accumulatedValue     = 0.0;
  _lastValue            = 0.0;
  _accumulatedSquares   = 0.0;
  _numberOfMeasurements = 0;
  _isAccurateValue      = false;
  _min                  = std::numeric_limits<double>::max();
  _max                  = -std::numeric_limits<double>::max();
  _minMeasurement       = -1;
  _maxMeasurement       = -1;
}

double tarch::instrumentation::Measurement::getMeanValue() const {
  assertion(_numberOfMeasurements > 0);
  return _accumulatedValue / static_cast<double>(_numberOfMeasurements);
}

double tarch::instrumentation::Measurement::getMeanValueOfNextStep(double newValue) const {
  assertion(_numberOfMeasurements > 0);
  return (_accumulatedValue + newValue) / static_cast<double>(_numberOfMeasurements + 1);
}

double tarch::instrumentation::Measurement::getAccumulatedValue() const { return _accumulatedValue; }

double tarch::instrumentation::Measurement::getMostRecentValue() const { return _lastValue; }

double tarch::instrumentation::Measurement::getValue() const {
  if (_numberOfMeasurements == 0) {
    assertionEquals(_accumulatedValue, 0.0);
    return 0.0;
  } else {
    return getMeanValue();
  }
}

double tarch::instrumentation::Measurement::getStandardDeviation() const {
  if (_numberOfMeasurements == 0) {
    assertionEquals(_accumulatedValue, 0.0);
    return 0.0;
  } else {
    const double variance = _accumulatedSquares / static_cast<double>(_numberOfMeasurements)
                            - getMeanValue() * getMeanValue();
    return std::sqrt(std::max(0.0, variance));
  }
}

bool tarch::instrumentation::Measurement::isAccurateValue() const { return _isAccurateValue; }

void tarch::instrumentation::Measurement::setAccuracy(const double& value) {
  assertion(value >= 0.0);
  _accuracy = value;
}

void tarch::instrumentation::Measurement::increaseAccuracy(const double& factor) {
  assertion(factor > 0.0);
  _accuracy /= factor;
}

void tarch::instrumentation::Measurement::setValue(const double& value) {
  if (_numberOfMeasurements <= 1) {
    assertion(!_isAccurateValue);
  } else {
    const double differenceDueToNewValue = tarch::la::smallerEquals(getMeanValue(), 1.0)
                                             ? std::abs(getMeanValueOfNextStep(value) - getMeanValue())
                                             : std::abs(getMeanValueOfNextStep(value) / getMeanValue() - 1.0);
    _isAccurateValue                     = _numberOfMeasurements > 0 && differenceDueToNewValue < _accuracy;
  }

  if (_min > value) {
    _min            = value;
    _minMeasurement = _numberOfMeasurements;
  }
  if (_max < value) {
    _max            = value;
    _maxMeasurement = _numberOfMeasurements;
  }

  _accumulatedValue += value;
  _accumulatedSquares += value * value;
  _numberOfMeasurements += 1;
  _lastValue = value;
}

int tarch::instrumentation::Measurement::getNumberOfMeasurements() const { return _numberOfMeasurements; }

std::string tarch::instrumentation::Measurement::toString() const {
  std::ostringstream msg;
  msg << "(mean=";
  if (_numberOfMeasurements > 0) {
    msg << getValue();
  } else {
    msg << "nan";
  }

  msg << ",n=" << _numberOfMeasurements;

  if (_numberOfMeasurements > 0) {
    msg << ",min=" << _min << "@" << _minMeasurement << ",max=" << _max << "@" << _maxMeasurement;
  } else {
    msg << ",min=nan,max=nan";
  }

  if (_numberOfMeasurements > 0) {
    const double value = getValue();
    if (!tarch::la::equals(value, 0.0)) {
      msg << ",rel+=" << (_max - value) / value * 100.0 << "%"
          << ",rel-=" << (value - _min) / value * 100.0 << "%";
    }
    msg << ",std=" << getStandardDeviation();
  }

  if (_accuracy > 0.0) {
    msg << ",eps=" << _accuracy;
    msg << ",status=" << (_isAccurateValue ? "converged" : "pending");
  }

  msg << ")";

  return msg.str();
}

double tarch::instrumentation::Measurement::max() const {
  assertion1(isAccurateValue(), toString());
  return _max;
}

double tarch::instrumentation::Measurement::min() const {
  assertion1(isAccurateValue(), toString());
  return _min;
}

int tarch::instrumentation::Measurement::getMinMeasurement() const { return _minMeasurement; }

int tarch::instrumentation::Measurement::getMaxMeasurement() const { return _maxMeasurement; }

double tarch::instrumentation::Measurement::getAccuracy() const { return _accuracy; }
