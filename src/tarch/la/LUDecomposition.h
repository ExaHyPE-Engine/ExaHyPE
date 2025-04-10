// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/la/DynamicMatrix.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"

#ifdef UseLapack
#include <lapacke.h>
#endif


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

#ifdef UseLapack
    /**
     * Template specialisation for the double type if Lapack is enabled.
     * \see \ref page_third_party_home
     * Accepts a square matrix R and inverts it using BLAS routines.
     * See documentation on these functions
     * [here](https://www.netlib.org/lapack/explore-html-3.6.1/dd/d9a/group__double_g_ecomputational_ga0019443faea08275ca60a734d0593e60.html)
     * and
     * [here](https://www.netlib.org/lapack/explore-html-3.6.1/dd/d9a/group__double_g_ecomputational_ga56d9c860ce4ce42ded7f914fdb0683ff.html)
     * These functions accept a pointer to a double array, which we get
     * from tarch::la::Matrix::data().
     *
     * This routine has two steps:
     * Step 1 - compute LU factorisation. M -> P * L * U, where P a permutation
     * matrix. This uses LAPACKE_dgetrf, and we check the return code is 0
     * before proceeding. Return code 0 means success, but return code >= 0
     * means that factorisation has occurred, but the matrix is singular.
     *
     * Step 2 - use the factored matrix as input to LAPACKE_dgetri. Again we
     * assert that the return code was 0.
     *
     */
    template <int Rows>
    Matrix<Rows, Rows, double> invert(const Matrix<Rows, Rows, double>& M);
#endif

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
