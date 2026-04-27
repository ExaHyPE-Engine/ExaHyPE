// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Matrix.h"
#include "Scalar.h"

namespace tarch::la {
  /**
   * Performs a matrix x matrix multiplication.
   *
   * The resulting matrix has to be created outside and given as a parameter.
   */
  template <int Rows, int Cols, int X, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> multiply(
    const Matrix<Rows, X, Scalar>& lhs,
    const Matrix<X, Cols, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> multiply(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> multiply(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> multiply(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, int X, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> multiplyComponents(
    const Matrix<Rows, X, Scalar>& lhs,
    const Matrix<X, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> multiplyComponents(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> multiplyComponents(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> multiplyComponents(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, int X, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> operator*(
    const Matrix<Rows, X, Scalar>& lhs,
    const Matrix<X, Cols, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> operator*(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> operator*(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> operator*(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs
  );

  /**
   * Bitwise comparison of the components of two matrices on equality.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const Matrix<Rows, Cols, Scalar>& lhs,
    const Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(const Matrix<2, 2, Scalar>& lhs, const Matrix<2, 2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(const Matrix<3, 3, Scalar>& lhs, const Matrix<3, 3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(const Matrix<4, 4, Scalar>& lhs, const Matrix<4, 4, Scalar>& rhs);

  /**
   * Compares to matrices on equality by means of a numerical accuracy.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Matrix<Rows, Cols, Scalar>& lhs,
    const Matrix<Rows, Cols, Scalar>& rhs,
    const Scalar&                     tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs,
    const Scalar&               tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs,
    const Scalar&               tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs,
    const Scalar&               tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> operator+(
    const Matrix<Rows, Cols, Scalar>& lhs,
    const Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> operator+(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> operator+(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> operator+(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> operator-(
    const Matrix<Rows, Cols, Scalar>& lhs,
    const Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> operator-(
    const Matrix<2, 2, Scalar>& lhs,
    const Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> operator-(
    const Matrix<3, 3, Scalar>& lhs,
    const Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> operator-(
    const Matrix<4, 4, Scalar>& lhs,
    const Matrix<4, 4, Scalar>& rhs
  );

  /**
   * Return Index of element which is not equals.
   *
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD std::pair<int, int> equalsReturnIndex(
    const Matrix<Rows, Cols, Scalar>& lhs,
    const Matrix<Rows, Cols, Scalar>& rhs,
    const Scalar&                     tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
} // namespace tarch::la

#include "tarch/la/MatrixMatrixOperations.cpph"
