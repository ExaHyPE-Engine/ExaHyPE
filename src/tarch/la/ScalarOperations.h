// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Scalar.h"

TARGET_DECLARE
namespace tarch::la {
  INLINE_METHOD GPU_CALLABLE_METHOD double abs(double value);
  INLINE_METHOD GPU_CALLABLE_METHOD int abs(int value);

  INLINE_METHOD GPU_CALLABLE_METHOD bool greater(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD GPU_CALLABLE_METHOD bool greaterEquals(
    double lhs,
    double rhs,
    double tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);
  INLINE_METHOD GPU_CALLABLE_METHOD bool smaller(double lhs, double rhs, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD GPU_CALLABLE_METHOD bool smallerEquals(
    double lhs,
    double rhs,
    double tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  INLINE_METHOD GPU_CALLABLE_METHOD int sign(double value, double tolerance = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD GPU_CALLABLE_METHOD int round(double value);
  INLINE_METHOD GPU_CALLABLE_METHOD int round(float value);
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/ScalarOperations.cpph"
