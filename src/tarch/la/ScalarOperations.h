// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Scalar.h"

TARGET_DECLARE

namespace tarch::la {
  INLINE_METHOD GPU_CALLABLE_METHOD double abs(double value);

  INLINE_METHOD GPU_CALLABLE_METHOD int abs(int value);

  /**
   * Returns a raised to the power i in integer arithmetic.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int aPowI(int i, int a);

  /**
   * @param tolerance Absolute tolerance when we compare two values.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD bool greater(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  /**
   * @param tolerance Absolute tolerance when we compare two values.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD bool greaterEquals(
    double lhs,
    double rhs,
    double tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * @param tolerance Absolute tolerance when we compare two values.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  /**
   * @param tolerance Absolute tolerance when we compare two values.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD bool smaller(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  /**
   * @param tolerance Absolute tolerance when we compare two values.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD bool smallerEquals(
    double lhs,
    double rhs,
    double tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * @return -1, 0, or 1 depending on the sign of value
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int sign(double value, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD GPU_CALLABLE_METHOD int round(double value);
  INLINE_METHOD GPU_CALLABLE_METHOD int round(float value);
} // namespace tarch::la

#include "tarch/la/ScalarOperations.cpph"

TARGET_DECLARE_END
