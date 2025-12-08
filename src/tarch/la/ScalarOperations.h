// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Scalar.h"

#include "tarch/tarch.h"
#include "tarch/accelerator/accelerator.h"

#include <cmath>

namespace tarch {
  namespace la {
    /**
     * Returns the absolute value of a type by redirecting to std::abs.
     */
    inline GPU_CALLABLE_METHOD
    double abs(double value);

    /**
     * Returns the absolute value of the given int.
     */
    inline GPU_CALLABLE_METHOD
    int abs(int value);

    /**
     * Computes the i-th power of a in integer arithmetic.
     */
    inline GPU_CALLABLE_METHOD
    int aPowI(int i, int a);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPU_CALLABLE_METHOD
    bool greater(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPU_CALLABLE_METHOD
    bool greaterEquals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPU_CALLABLE_METHOD
    bool equals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPU_CALLABLE_METHOD
    bool smaller(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPU_CALLABLE_METHOD
    bool smallerEquals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * @return -10, or 1 depending on the sign of value
     */
    inline GPU_CALLABLE_METHOD
    int sign(double value, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

    inline GPU_CALLABLE_METHOD
    int round(double value);
    inline GPU_CALLABLE_METHOD
    int round(float value);
  }
}

#include "ScalarOperations.cpph"
