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
     * We originally had the signature
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * static bool smaller( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod {
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * but that one gives a compiler warning:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * ./tarch/la/ScalarOperations.h:63:96: warning: GCC does not allow 'always_inline' attribute in this position on a function definition [-Wgcc-compat]
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * It seems that the inlining flag has to be used prior to the result data
     * type, if we also add the implementation to the header.
     *
     * Further to that, we add maybe_unused, as the compiler otherwise warns
     * about the inline function unused if no extension is enabled that uses
     * it.
     *
     * @param tolerance Absolute tolerance when we compare two values.
     */
    [[maybe_unused]]
    static InlineMethod bool smaller( double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) {
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


