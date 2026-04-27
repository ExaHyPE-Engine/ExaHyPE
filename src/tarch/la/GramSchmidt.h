// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/Matrix.h"

#include <cmath>

namespace tarch::la {
  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD void modifiedGramSchmidt(
    Matrix<Rows, Cols, Scalar>  A,
    Matrix<Rows, Cols, Scalar>& Q,
    Matrix<Cols, Cols, Scalar>& R
  );

  void modifiedGramSchmidt(DynamicMatrix A, DynamicMatrix& Q, DynamicMatrix& R);
} // namespace tarch::la

#include "tarch/la/GramSchmidt.cpph"
