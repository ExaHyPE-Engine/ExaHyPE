// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"

namespace tarch::la {
  /**
   * Computes the sum of all entries of the matrix.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Matrix<Rows, Cols, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Matrix<4, 4, Scalar>& matrix);

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Cols, Rows, Scalar> transpose(const Matrix<Rows, Cols, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> transpose(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> transpose(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> transpose(const Matrix<4, 4, Scalar>& matrix);

  template <int Rows, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD static Matrix<Rows, Rows, Scalar> identity();

  template <int Rows, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Rows, Scalar> diag(const Matrix<Rows, Rows, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> diag(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> diag(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> diag(const Matrix<4, 4, Scalar>& matrix);

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Cols, Scalar> row(const Matrix<Rows, Cols, Scalar>& matrix, int whichRow);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> row(const Matrix<2, 2, Scalar>& matrix, int whichRow);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> row(const Matrix<3, 3, Scalar>& matrix, int whichRow);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> row(const Matrix<4, 4, Scalar>& matrix, int whichRow);

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Rows, Scalar> col(const Matrix<Rows, Cols, Scalar>& matrix, int whichColumn);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> col(const Matrix<2, 2, Scalar>& matrix, int whichColumn);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> col(const Matrix<3, 3, Scalar>& matrix, int whichColumn);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> col(const Matrix<4, 4, Scalar>& matrix, int whichColumn);

  /**
   * Returns a matrix that holds the reciprocal of every entry.
   *
   * @see tarch::la::invert()
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Rows, Cols, Scalar> invertEntries(const Matrix<Rows, Cols, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<2, 2, Scalar> invertEntries(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<3, 3, Scalar> invertEntries(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<4, 4, Scalar> invertEntries(const Matrix<4, 4, Scalar>& matrix);

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar frobeniusNorm(const Matrix<Rows, Cols, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar frobeniusNorm(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar frobeniusNorm(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar frobeniusNorm(const Matrix<4, 4, Scalar>& matrix);

  /**
   * Returns the largest absolute entry of the matrix.
   */
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar elementMax(const Matrix<Rows, Cols, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar elementMax(const Matrix<2, 2, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar elementMax(const Matrix<3, 3, Scalar>& matrix);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar elementMax(const Matrix<4, 4, Scalar>& matrix);
} // namespace tarch::la

template <int Rows, int Cols, typename Scalar>
INLINE_METHOD std::ostream& operator<<(std::ostream& os, const tarch::la::Matrix<Rows, Cols, Scalar>& matrix);

#include "tarch/la/MatrixOperations.cpph"
