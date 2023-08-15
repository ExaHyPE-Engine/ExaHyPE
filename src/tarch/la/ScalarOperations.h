// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tarch.h"
#include "tarch/la/Scalar.h"

#pragma once

#pragma once


#include <complex>


namespace tarch {
  namespace la {

    /**
     * Returns the absolute value of a type by redirecting to std::abs.
     */
    double abs( double value);

    /**
     * Returns the absolute value of the given int.
     */
    int abs (int value);

    double abs( const std::complex<double>& value );

    /**
     * Computes the i-th power of a in integer arithmetic.
     */
    int aPowI(int i,int a);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    bool greater( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    bool greaterEquals( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    bool equals( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * Smaller operator for floating point values
     *
     * This operation is a header-only operation on purpose, as we use it in
     * some SPH compute kernels which we want to vectorise aggressively.
     *
     * The static here is required to avoid multiple definition errors.
     *
     * @param tolerance Absolute tolerance when we compare two values.
     */
    static bool smaller( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod {
      return lhs - rhs < -tolerance;
    }

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    bool smallerEquals( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    bool equals( const std::complex<double>& lhs, const std::complex<double>& rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @return -10, or 1 depending on the sign of value
     */
    int sign(double value, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    int round(double value);
    int round(float value);
  }
}


