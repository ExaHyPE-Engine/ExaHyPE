// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::instrumentation {
  class Measurement;
} // namespace tarch::instrumentation

/**
 * Measurement
 *
 * The shared memory oracles have to handle real time measurements. This data
 * is always noisy, inaccurate, and has to be averaged. We originally planned
 * to move all the averaging etc. to the oracle singleton. However, that is
 * hardly possible, as the responsible oracle changes all the time, and the
 * measurements are associated to one oracle. Consequently, we introduced this
 * helper class for the individual oracle implementations.
 *
 * @author Tobias Weinzierl
 */
class tarch::instrumentation::Measurement {
private:
  double _accuracy;

  /**
   * Accumulated value
   *
   * The sum of all values divided by _numberOfMeasurements gives the mean
   * value.
   */
  double _accumulatedValue;

  /**
   * We store the last value.
   */
  double _lastValue;

  /**
   * To compute the standard deviation, we rely on the formula
   *
   * sigma = sqrt(E(x^2) - E(x)^2)
   *
   * with E being the mean value.
   */
  double _accumulatedSquares;

  /**
   * Needed to compute average value and variance.
   */
  int    _numberOfMeasurements;
  bool   _isAccurateValue;
  double _min;
  double _max;
  int    _minMeasurement;
  int    _maxMeasurement;

  double getAverageValueOfNextStep(double newValue) const;

public:
  Measurement(const double& accuracy = 0.0);
  ~Measurement() = default;

  /**
   * @return Averaged value of all measurements. If you have not yet added a
   *   value, it returns 0.
   */
  double getAverageValue() const;

  /**
   * @return Last value added. If there's none so far, the routine returns 0.
   *   If you've only added one value so far, the result equals
   *   getAverageValue().
   */
  double getMostRecentValue() const;

  /**
   * @return Sum over all values measured so far.
   */
  double getAccumulatedValue() const;

  /**
   * Clamp small negative rounding errors in the variance to zero.
   */
  double getStandardDeviation() const;

  /**
   * Is value accurate
   *
   * Whether a value is accurate depends on the last setValue() call. The
   * class internally holds the mean value of all setValue() calls. If a new
   * value is set/added, the object checks whether this additional
   * measurement modifies the mean value more than the given accuracy. If
   * this is the case, the object does not represent a valid measurement yet.
   *
   * It does not work if we just compare new values to the mean value. If a
   * measurement looks similar to the sketch from above (black=mean value,
   * grey area=accuracy, blue points=values), the measurement never would
   * become valid.
   *
   * I therefore use a normalised mean value
   *
   * @f$ valid = | \frac{ m^{(n+1)} - m^{(n)} }{m^{(n)}} | \leq \epsilon = | \frac{m^{(n+1)}}{m^{(n)}}-1  | \leq
   * \epsilon @f$
   *
   * where @f$ m^{(n)} @f$ and @f$ m^{(n+1)} @f$ are two subsequent mean
   * values.
   */
  bool isAccurateValue() const;

  /**
   * @see isAccurateValue()
   */
  void setAccuracy(const double& value);

  void increaseAccuracy(const double& factor);

  /**
   * Set the value. If the measurement already holds a value, this value is
   * not overwritten. Instead, the measurement accumulates all values and
   * returns the average.
   */
  void setValue(const double& value);
  int  getNumberOfMeasurements() const;

  double max() const;
  double min() const;

  /**
   * @return The index of the measurement at which the minimum value occurred.
   */
  int getMinMeasurement() const;

  /**
   * @return The index of the measurement at which the maximum value occurred.
   */
  int getMaxMeasurement() const;

  void erase();

  double getAccuracy() const;
};
