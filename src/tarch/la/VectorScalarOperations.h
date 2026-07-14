// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "ScalarOperations.h"
#include "Vector.h"

namespace tarch::la {
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator*=(
    Vector<Size, Scalar>& vector,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator*=(Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator*=(Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator*=(Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator/=(
    Vector<Size, Scalar>& vector,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator/=(Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator/=(Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator/=(Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator+=(
    Vector<Size, Scalar>& vector,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator+=(Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator+=(Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator+=(Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator-=(
    Vector<Size, Scalar>& vector,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator-=(Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator-=(Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator-=(Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator*(
    const Vector<Size, Scalar>& vector,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator*(const Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator*(const Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator*(const Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator*(
    const Scalar&               scalar,
    const Vector<Size, Scalar>& vector
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator/(
    const Vector<Size, Scalar>& vector,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator/(const Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator/(const Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator/(const Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator+(
    const Vector<Size, Scalar>& vector,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator+(const Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator+(const Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator+(const Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator+(
    const Scalar&               scalar,
    const Vector<Size, Scalar>& vector
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator-(
    const Vector<Size, Scalar>& vector,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator-(const Vector<2, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator-(const Vector<3, Scalar>& vector, const Scalar& scalar);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator-(const Vector<4, Scalar>& vector, const Scalar& scalar);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneEquals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const Vector<Size, Scalar>& lhs,
    const Scalar&               cmp,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const Vector<2, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const Vector<3, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const Vector<4, Scalar>& lhs,
    const Scalar&            cmp,
    const Scalar             tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Return the remainder of a division
   *
   * Per component x_i, the function computes
   * x_i/h and then returns the remaining fraction of x_i which did
   * not fit into h. The function therefore resembles modulo.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> remainder(const Vector<Size, Scalar>& vector, double h);
} // namespace tarch::la

#include "VectorScalarOperations.cpph"
