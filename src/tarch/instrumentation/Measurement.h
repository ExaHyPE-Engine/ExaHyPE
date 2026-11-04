// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::instrumentation {
  class Measurement;
} // namespace tarch::instrumentation

/**
 * Accumulates the values of one repeated measurement. A caller adds one value per repetition and
 * reads their sum, their count and their mean.
 */
class tarch::instrumentation::Measurement final {
public:
  Measurement()                              = default;
  Measurement(const Measurement&)            = default;
  Measurement(Measurement&&)                 = default;
  Measurement& operator=(const Measurement&) = default;
  Measurement& operator=(Measurement&&)      = default;
  ~Measurement()                             = default;

  double getAverageValue() const;
  double getAccumulatedValue() const;
  void setValue(const double& value);
  int getNumberOfMeasurements() const;

private:
  double _accumulatedValue{0.0};
  int    _numberOfMeasurements{0};
};
