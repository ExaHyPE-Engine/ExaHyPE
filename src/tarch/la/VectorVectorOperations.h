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
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator+=(
    tarch::la::Vector<Size, Scalar>&       lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator+=(
    tarch::la::Vector<2, Scalar>&       lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator+=(
    tarch::la::Vector<3, Scalar>&       lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator+=(
    tarch::la::Vector<4, Scalar>&       lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar>& operator-=(
    tarch::la::Vector<Size, Scalar>&       lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>& operator-=(
    tarch::la::Vector<2, Scalar>&       lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>& operator-=(
    tarch::la::Vector<3, Scalar>&       lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>& operator-=(
    tarch::la::Vector<4, Scalar>&       lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator+(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator+(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator+(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator+(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> operator-(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> operator-(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> operator-(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> operator-(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> multiplyComponents(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> multiplyComponents(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> multiplyComponents(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> multiplyComponents(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> divideComponents(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> divideComponents(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> divideComponents(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> divideComponents(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(
    const tarch::la::Vector<2, Scalar>& lhs,
    const tarch::la::Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(
    const tarch::la::Vector<4, Scalar>& lhs,
    const tarch::la::Vector<4, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> cross(
    const tarch::la::Vector<3, Scalar>& lhs,
    const tarch::la::Vector<3, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool firstGreater(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> max(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, Scalar> min(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator!=(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int equalsReturnIndex(
    const tarch::la::Vector<Size, Scalar>& lhs,
    const tarch::la::Vector<Size, Scalar>& rhs,
    Scalar                                 tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size>
  INLINE_METHOD GPU_CALLABLE_METHOD int equalsReturnIndex(
    const tarch::la::Vector<Size, int>& lhs,
    const tarch::la::Vector<Size, int>& rhs
  );
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/VectorVectorOperations.cpph"
