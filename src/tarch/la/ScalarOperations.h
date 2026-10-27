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

  template <typename Value>
  using BitPatternOf = std::conditional_t<sizeof(Value) == sizeof(std::uint64_t), std::uint64_t, std::uint32_t>;

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD BitPatternOf<Value> getOrderKeyOf(Value value);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD BitPatternOf<Value> getMagnitudeKeyOf(Value value);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isBelow(Value lhs, Value rhs);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isBelowInMagnitude(Value lhs, Value rhs);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isEqualInMagnitude(Value lhs, Value rhs);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void orderByMagnitude(Value& first, Value& second, Value& third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value sumInOrderOfMagnitude(Value first, Value second, Value third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value sumOfSquaresInOrderOfMagnitude(Value first, Value second, Value third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value productInOrderOfMagnitude(Value first, Value second, Value third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value productRoundedOnce(Value first, Value second);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void orderByValue(Value& first, Value& second, Value& third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value sumInOrderOfValue(Value first, Value second, Value third);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value meanInOrderOfValue(Value first, Value second, Value third);

  INLINE_METHOD GPU_CALLABLE_METHOD double sumInFixedPoint(const double* values, int size);

  template <int Count>
  INLINE_METHOD GPU_CALLABLE_METHOD double meanInFixedPoint(const double (&values)[Count]);
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/ScalarOperations.cpph"
