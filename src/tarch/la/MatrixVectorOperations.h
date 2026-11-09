// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace tarch::la {
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
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/MatrixVectorOperations.cpph"
