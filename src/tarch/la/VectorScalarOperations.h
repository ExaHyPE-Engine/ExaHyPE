// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace tarch::la {
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator*=(
    tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                    scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator*=(
    tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator*=(
    tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator*=(
    tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                 scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator/=(
    tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                    scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator/=(
    tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator/=(
    tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator/=(
    tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                 scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator+=(
    tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                    scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator+=(
    tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator+=(
    tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator+=(
    tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                 scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator-=(
    tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                    scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator-=(
    tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator-=(
    tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                 scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator-=(
    tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                 scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator*(
    const tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                          scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator*(
    const tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator*(
    const tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator*(
    const tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                       scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator*(
    const Scalar&                          scalar,
    const tarch::la::Vector<Size, Scalar>& vector
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator/(
    const tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                          scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator/(
    const tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator/(
    const tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator/(
    const tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                       scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator+(
    const tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                          scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator+(
    const tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator+(
    const tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator+(
    const tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                       scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator+(
    const Scalar&                          scalar,
    const tarch::la::Vector<Size, Scalar>& vector
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator-(
    const tarch::la::Vector<Size, Scalar>& vector,
    const Scalar&                          scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator-(
    const tarch::la::Vector<2, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator-(
    const tarch::la::Vector<3, Scalar>& vector,
    const Scalar&                       scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator-(
    const tarch::la::Vector<4, Scalar>& vector,
    const Scalar&                       scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const Scalar&                          cmp,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const tarch::la::Vector<2, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const tarch::la::Vector<3, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const tarch::la::Vector<4, Scalar>& lhs,
    const Scalar&                       cmp,
    Scalar                              tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> remainder(
    const tarch::la::Vector<Size, Scalar>& vector,
    double                                 h
  );
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/VectorScalarOperations.cpph"
