// Copyright (C) 2009 Technische Universitaet Muenchen
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <string>
#include <vector>

#include "tarch/logging/Log.h"


namespace tarch::timing {
  class GlidingAverageMeasurement;
} // namespace tarch::timing


/**
 * GlidingAverageMeasurement
 *
 * This is an alternative implementation to Measurement which uses a gliding
 * average rather than the real average over all data. The gliding average is
 * computed as follows:
 *
 * @f$ m = \frac{ \sum _i w^i \cdot m_i }{ \sum _i w^i } @f$
 *
 * This w is the weight from the constructor. The @f$ m_i @f$ are the
 * recorded measurements.
 *
 *
 * @author Tobias Weinzierl
 */
class tarch::timing::GlidingAverageMeasurement {
  private:
    static tarch::logging::Log _log;

    /**
     * The weights q determine the weight via $q^0,q^1,q^2,...$.
     */
    const double _weight;
    const int    _maxEntries;

    /**
     * Data
     */
    std::vector<double> _values;

  public:
    static constexpr double DefaultWeight = 0.7;

    /**
     * Construct a measurement with a gliding measurement
     *
     * @param weight Has to be 0<weight<=1.0. See class description for details
     *   how this value is used.
     * @param maxEntries How many entries have to be taken into account. Has to
     *   bigger than one. You can pass in 1, but then it is not a gliding
     *   average, but just a value.
     */
    GlidingAverageMeasurement(double weight = DefaultWeight, int maxEntries = 16);
    ~GlidingAverageMeasurement() = default;

    /**
     * Return the value of the gliding value
     *
     * The formula is described in the class documentation. Note that the
     * entries _values are ordered historically, i.e. _values[0] is the
     * oldest value. That means we have to do some index magic compared
     * to the sums in the class docu.
     *
     * @return Value repreented by this gliding average.
     */
    double getValue() const;

    /**
     * To compute the standard deviation, we rely on the formula
     *
     * sigma =sqrt( E(x^2) - E(x)^2 )
     *
     * with E being the weighted mean value.
     */
    double getStandardDeviation() const;

    /**
     * @return Number of setValue() calls has exceeded the maxEntries argument
     *   from the constructor
     */
    bool hasFullSetOfMeasurements() const;

    /**
     * Is value accurate
     *
     * A value is accurate if its standard deviation divided by its mean is smaller than the factor.
     */
    bool isAccurateValue(double factor = 0.1) const;

    /**
     * Set the value. If the measurement already holds a value, this value is
     * not overwritten. Instead, the measurement accumulates all values and
     * returns the average.
     */
    void setValue(const double& value);

    std::string toString() const;

    double max() const;
    double min() const;

    void erase();

    /**
     * @return How many values do feed into a single result, i.e. how many
     *         entries do we keep track of?
     */
    int getObservationSpan() const;
};
