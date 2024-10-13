// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Scalar.h"

#include "tarch/tarch.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <cmath>
#include <complex>

namespace tarch {
  namespace la {
    /**
     * Returns the absolute value of a type by redirecting to std::abs.
     */
    inline GPUCallableMethod
    double abs(double value) InlineMethod;

    /**
     * Returns the absolute value of the given int.
     */
    inline GPUCallableMethod
    int abs(int value) InlineMethod;

    inline GPUCallableMethod
    double abs(const std::complex<double>& value) InlineMethod;

    /**
     * Computes the i-th power of a in integer arithmetic.
     */
    inline GPUCallableMethod
    int aPowI(int i, int a) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool greater(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool greaterEquals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool equals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool smaller(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool smallerEquals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @param tolerance Absolute tolerance when we compare two values.
     */
    inline GPUCallableMethod
    bool equals(const std::complex<double>& lhs, const std::complex<double>& rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    /**
     * @return -10, or 1 depending on the sign of value
     */
    inline GPUCallableMethod
    int sign(double value, double tolerance = NUMERICAL_ZERO_DIFFERENCE) InlineMethod;

    inline GPUCallableMethod
    int round(double value) InlineMethod;

    inline GPUCallableMethod
    int round(float value) InlineMethod;
  }
}

#include "ScalarOperations.cpph"
