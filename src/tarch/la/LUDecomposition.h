// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/Matrix.h"

namespace tarch::la {
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD void backSubstitution(
    const Matrix<Size, Size, Scalar>& R,
    const Vector<Size, Scalar>&       f,
    Vector<Size, Scalar>&             x
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix<Size, Size, Scalar> invertUpperTriangular(
    const Matrix<Size, Size, Scalar>& R
  );

  void backSubstitution(const DynamicMatrix& R, const double* f, double* x);

  DynamicMatrix invertUpperTriangular(const DynamicMatrix& R);
} // namespace tarch::la

#include "tarch/la/LUDecomposition.cpph"
