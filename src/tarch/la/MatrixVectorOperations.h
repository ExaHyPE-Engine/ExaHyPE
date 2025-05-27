// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
      const Matrix<Rows, Cols, Scalar>& matrix,
      const Vector<Cols, Scalar>&       vector
    );

    /**
     * Compute r = r + alpha * M * v in-situ
     */
    template <int Rows, int Cols, typename Scalar>
    void matVec(
      const Matrix<Rows, Cols, Scalar>& matrix,
      Scalar                            alpha,
      const Vector<Cols, Scalar>&       v,
      Vector<Rows, Scalar>&             r
    );

    /**
     * Performs a matrix x vector multiplication.
     *
     * The result vector is created inside, multiply is used to execute the
     * multiplication.
     */
    template <int Rows, int Cols, typename Scalar>
    Vector<Rows, Scalar> operator*(
      const Matrix<Rows, Cols, Scalar>& matrix,
      const Vector<Cols, Scalar>&       vector
    );

#ifdef UseOpenblas
    /**
     * Include template specialisation for double if we are using Openblas.
     * \see \ref page_third_party_home
     */
    template <int Rows, int Cols>
    Vector<Rows, double> operator*(
      const Matrix<Rows, Cols, double>& matrix,
      const Vector<Cols, double>&       vector
    );
#endif

    /**
     * Outer dot product.
     */
    template <int Size, typename Scalar>
    Matrix<Size, Size, Scalar> outerDot(
      const Vector<Size, Scalar>& lhs,
      const Vector<Size, Scalar>& rhs
    );
  } // namespace la
} // namespace tarch


#include "MatrixVectorOperations.cpph"
