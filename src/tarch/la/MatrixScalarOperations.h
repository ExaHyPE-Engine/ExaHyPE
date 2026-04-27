// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Matrix.h"

namespace tarch::la {
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar>& operator*=(
    Matrix<Rows, Cols, Scalar>& matrix,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar>& operator*=(
    Matrix<2, 2, Scalar>& matrix,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar>& operator*=(
    Matrix<3, 3, Scalar>& matrix,
    const Scalar&         scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar>& operator*=(
    Matrix<4, 4, Scalar>& matrix,
    const Scalar&         scalar
  );

  /**
   * Multiplies every component of the matrix with the scalar
   *
   * No temporary objects are created during the operation.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> operator*(
    const Matrix<Rows, Cols, Scalar>& matrix,
    const Scalar&                     scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> operator*(
    const Matrix<2, 2, Scalar>& matrix,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> operator*(
    const Matrix<3, 3, Scalar>& matrix,
    const Scalar&               scalar
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> operator*(
    const Matrix<4, 4, Scalar>& matrix,
    const Scalar&               scalar
  );

  /**
   * Multiplies every component of the matrix with the scalar
   *
   * No temporary objects are created during the operation.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> operator*(
    const Scalar&                     scalar,
    const Matrix<Rows, Cols, Scalar>& matrix
  );
} // namespace tarch::la

#include "tarch/la/MatrixScalarOperations.cpph"
