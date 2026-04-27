// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"

namespace tarch::la {
  /**
   * Performs a matrix x vector multiplication.
   *
   * The result vector has to be created outside and given as a parameter.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Rows, Scalar> multiply(
    const Matrix<Rows, Cols, Scalar>& matrix,
    const Vector<Cols, Scalar>&       vector
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> multiply(
    const Matrix<2, 2, Scalar>& matrix,
    const Vector<2, Scalar>&    vector
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> multiply(
    const Matrix<3, 3, Scalar>& matrix,
    const Vector<3, Scalar>&    vector
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> multiply(
    const Matrix<4, 4, Scalar>& matrix,
    const Vector<4, Scalar>&    vector
  );

  /**
   * Compute r = r + alpha * M * v in-situ
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD void matVec(
    const Matrix<Rows, Cols, Scalar>& matrix,
    Scalar                            alpha,
    const Vector<Cols, Scalar>&       v,
    Vector<Rows, Scalar>&             r
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD void matVec(
    const Matrix<2, 2, Scalar>& matrix,
    Scalar                      alpha,
    const Vector<2, Scalar>&    v,
    Vector<2, Scalar>&          r
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD void matVec(
    const Matrix<3, 3, Scalar>& matrix,
    Scalar                      alpha,
    const Vector<3, Scalar>&    v,
    Vector<3, Scalar>&          r
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD void matVec(
    const Matrix<4, 4, Scalar>& matrix,
    Scalar                      alpha,
    const Vector<4, Scalar>&    v,
    Vector<4, Scalar>&          r
  );

  /**
   * Performs a matrix x vector multiplication.
   *
   * The result vector is created inside, multiply is used to execute the
   * multiplication.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Rows, Scalar> operator*(
    const Matrix<Rows, Cols, Scalar>& matrix,
    const Vector<Cols, Scalar>&       vector
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator*(
    const Matrix<2, 2, Scalar>& matrix,
    const Vector<2, Scalar>&    vector
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator*(
    const Matrix<3, 3, Scalar>& matrix,
    const Vector<3, Scalar>&    vector
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator*(
    const Matrix<4, 4, Scalar>& matrix,
    const Vector<4, Scalar>&    vector
  );

  /**
   * Outer dot product.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Size, Size, Scalar> outerDot(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );
} // namespace tarch::la

#include "MatrixVectorOperations.cpph"
