// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Scalar.h"

TARGET_DECLARE
namespace tarch::la {
  template <int Rows, int Cols, int X, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, Scalar> multiply(
    const tarch::la::Matrix<Rows, X, Scalar>& lhs,
    const tarch::la::Matrix<X, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, Scalar> multiply(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<3, 3, Scalar> multiply(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, Scalar> multiply(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, Scalar> multiplyComponents(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, Scalar> multiplyComponents(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<3, 3, Scalar> multiplyComponents(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, Scalar> multiplyComponents(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, int X, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, Scalar> operator*(
    const tarch::la::Matrix<Rows, X, Scalar>& lhs,
    const tarch::la::Matrix<X, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, Scalar> operator*(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<3, 3, Scalar> operator*(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, Scalar> operator*(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs,
    const Scalar&                                tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs,
    const Scalar&                          tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs,
    const Scalar&                          tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs,
    const Scalar&                          tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, Scalar> operator+(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, Scalar> operator+(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<3, 3, Scalar> operator+(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, Scalar> operator+(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, Scalar> operator-(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, Scalar> operator-(
    const tarch::la::Matrix<2, 2, Scalar>& lhs,
    const tarch::la::Matrix<2, 2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<3, 3, Scalar> operator-(
    const tarch::la::Matrix<3, 3, Scalar>& lhs,
    const tarch::la::Matrix<3, 3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, Scalar> operator-(
    const tarch::la::Matrix<4, 4, Scalar>& lhs,
    const tarch::la::Matrix<4, 4, Scalar>& rhs
  );

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD std::pair<int, int> equalsReturnIndex(
    const tarch::la::Matrix<Rows, Cols, Scalar>& lhs,
    const tarch::la::Matrix<Rows, Cols, Scalar>& rhs,
    const Scalar&                                tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  );
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/MatrixMatrixOperations.cpph"
