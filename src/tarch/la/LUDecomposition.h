// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"

namespace tarch {
  namespace la {
    /**
     * Performs an in-situ LU-decomposition of the square matrix A. Returns
     * pivot values, too. The storage format is normalised such that the
     * diagonal values of the lower triangular matrix are one.
     */
    template <int Rows, typename Scalar>
    void lu(Matrix<Rows, Rows, Scalar>& A, Vector<Rows, int>& pivots);

    /**
     * In-situ LU without pivoting. See the other LU routine.
     */
    template <int Rows, typename Scalar> void lu(Matrix<Rows, Rows, Scalar>& A);

    /**
     * Back substitution following LU decomposition
     *
     * Accepts an upper triangular matrix and a rhs. It then returns the
     * solution x to @f$ Rx=f @f$ i.e. @f$  x=R^{-1}f @f$. We assume that
     * R is a proper, non-normalised upper triangular matrix. It does not
     * have to have 1s on the diagonals.
     *
     * @param R the upper matrix from the LU decomposition
     */
    template <int Rows, typename Scalar>
    Vector<Rows, Scalar> backSubstitution(
      const Matrix<Rows, Rows, Scalar>& R,
      const Vector<Rows, Scalar>&       f
    );

    void backSubstitution(const DynamicMatrix& R, const double* f, double* x);

    DynamicMatrix invertUpperTriangular(const DynamicMatrix& R);

    /**
     * Invert matrix with LU decomposition
     *
     * We first invoke the LU decomposition through a call to lu(). After
     * that, we compute the mat vec and return the result. This is not an
     * optimised version in any way. For small matrices, it should be pretty
     * fast. For larger matrices, you might want to consider the Lapack
     * version available through the alternative invert() version.
     */
    template <int Rows, typename Scalar>
    Matrix<Rows, Rows, Scalar> invert(const Matrix<Rows, Rows, Scalar>& M);

    /**
     * Specialisation of inversion
     *
     * This one inverts directly and is usually faster than BLAS.
     */
    template <typename Scalar>
    Matrix<2, 2, Scalar> invert(const Matrix<2, 2, Scalar>& M);

    /**
     * Specialisation of inversion
     *
     * This one inverts directly and is usually faster than BLAS.
     */
    template <typename Scalar>
    Matrix<3, 3, Scalar> invert(const Matrix<3, 3, Scalar>& M);

    template <typename Scalar> double det(const Matrix<2, 2, Scalar>& R);

    template <typename Scalar> double det(const Matrix<3, 3, Scalar>& R);

  } // namespace la
} // namespace tarch


#include "tarch/la/LUDecomposition.cpph"
