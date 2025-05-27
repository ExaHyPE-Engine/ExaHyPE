// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/ShallowMatrix.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"


namespace tarch {
  namespace la {
    /**
     * Performs a matrix x vector multiplication.
     *
     * The result vector has to be created outside and given as a parameter.
     */
    template <int Rows, int Cols, typename Scalar>
    Vector<Rows, Scalar> multiply(
      const ShallowMatrix<Rows, Cols, Scalar>& matrix,
      const Vector<Cols, Scalar>&       vector
    );

    template <int Rows, int Cols, typename Scalar>
    Vector<Rows, Scalar> multiply(
      const ShallowMatrix<Rows, Cols, Scalar>&   matrix,
      const SmartPointerVector<Cols, Scalar>&    vector
    );
  } // namespace la
} // namespace tarch


#include "ShallowMatrixVectorOperations.cpph"
