// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

namespace tarch::la {
  /**
   * Adds rhs to lhs and assigns the result to lhs.
   *
   * No temporary vector is created.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator+=(
    Vector<Size, Scalar>&       lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator+=(Vector<2, Scalar>& lhs, const Vector<2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator+=(Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator+=(Vector<4, Scalar>& lhs, const Vector<4, Scalar>& rhs);

  /**
   * Subtracts rhs from lhs and assigns the result to lhs.
   *
   * No temporary vector is created.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar>& operator-=(
    Vector<Size, Scalar>&       lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar>& operator-=(Vector<2, Scalar>& lhs, const Vector<2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar>& operator-=(Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar>& operator-=(Vector<4, Scalar>& lhs, const Vector<4, Scalar>& rhs);

  /**
   * Adds rhs to lhs.
   *
   * Returns the result in a temporary vector.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator+(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator+(
    const Vector<2, Scalar>& lhs,
    const Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator+(
    const Vector<3, Scalar>& lhs,
    const Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator+(
    const Vector<4, Scalar>& lhs,
    const Vector<4, Scalar>& rhs
  );

  /**
   * Subtracts rhs from lhs.
   *
   * Returns the result in a temporary vector.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator-(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> operator-(
    const Vector<2, Scalar>& lhs,
    const Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> operator-(
    const Vector<3, Scalar>& lhs,
    const Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> operator-(
    const Vector<4, Scalar>& lhs,
    const Vector<4, Scalar>& rhs
  );

  /**
   * Multiplies lhs and rhs component by component and returns the result.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> multiplyComponents(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> multiplyComponents(
    const Vector<2, Scalar>& lhs,
    const Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> multiplyComponents(
    const Vector<3, Scalar>& lhs,
    const Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> multiplyComponents(
    const Vector<4, Scalar>& lhs,
    const Vector<4, Scalar>& rhs
  );

  /**
   * Divides lhs by rhs component by component and returns the result.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> divideComponents(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> divideComponents(
    const Vector<2, Scalar>& lhs,
    const Vector<2, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> divideComponents(
    const Vector<3, Scalar>& lhs,
    const Vector<3, Scalar>& rhs
  );
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> divideComponents(
    const Vector<4, Scalar>& lhs,
    const Vector<4, Scalar>& rhs
  );

  /**
   * Performs the dot (=inner) product of two vectors.
   *
   * For the outer dot product consult the file MatrixVectorOperations.h.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(const Vector<Size, Scalar>& lhs, const Vector<Size, Scalar>& rhs);

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(const Vector<2, Scalar>& lhs, const Vector<2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(const Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar operator*(const Vector<4, Scalar>& lhs, const Vector<4, Scalar>& rhs);

  /**
   * Performs the dot (=inner) product of two vectors.
   *
   * For the outer dot product consult the file MatrixVectorOperations.h.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(const Vector<Size, Scalar>& lhs, const Vector<Size, Scalar>& rhs);

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(const Vector<2, Scalar>& lhs, const Vector<2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(const Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar dot(const Vector<4, Scalar>& lhs, const Vector<4, Scalar>& rhs);

  /**
   * Performs the dot (=inner) product of two vectors.
   *
   * For the outer dot product consult the file MatrixVectorOperations.h.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(const Vector<Size, Scalar>& lhs, const Vector<Size, Scalar>& rhs);

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(const Vector<2, Scalar>& lhs, const Vector<2, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(const Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar innerDot(const Vector<4, Scalar>& lhs, const Vector<4, Scalar>& rhs);

  /**
   * Returns the cross product of two 3D vectors.
   */
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> cross(const Vector<3, Scalar>& lhs, const Vector<3, Scalar>& rhs);

  /**
   * Compares two vectors for equality up to a numerical accuracy.
   *
   * @param tolerance Absolute tolerance. If you prefer to use a relative
   *  measure, you might want to use tarch::la::relativeEps()
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool equals(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Compares sequentially every component pair of lhs and rhs, and stops
   * as soon as one is greater than the other.
   *
   * Defines an absolute pairwise ordering between (unequal) vectors.
   *
   * @param tolerance Absolute tolerance. If you prefer to use a relative
   *  measure, you might want to use tarch::la::relativeEps()
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool firstGreater(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Returns true, if one component of lhs is greater than the corresponding
   * component in rhs up to numerical accuracy.
   *
   * @param tolerance Absolute tolerance. If you prefer to use a relative
   *  measure, you might want to use tarch::la::relativeEps()
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreater(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Returns true, if one component of lhs is greater or equals than the
   * corresponding component in rhs up to numerical accuracy.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneGreaterEquals(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Returns true, if all components of lhs are greater than the corresponding
   * components in rhs up to numerical accuracy.
   *
   * @param tolerance Absolute tolerance. If you prefer to use a relative
   *  measure, you might want to use tarch::la::relativeEps()
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreater(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Returns true, if all components of lhs are greater or equals than the corresponding
   * components in rhs up to numerical accuracy.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allGreaterEquals(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmaller(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allSmallerEquals(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmaller(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool oneSmallerEquals(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Component-wise max evaluation
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> max(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  /**
   * Component-wise min evaluation
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> min(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs
  );

  /**
   * Bit-wise comparison of the components of two vectors for equality.
   *
   * This method should not be used for floating-point valued vectors. Instead,
   * equals() is the suitable comparison.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator==(const Vector<Size, Scalar>& lhs, const Vector<Size, Scalar>& rhs);

  /**
   * Bit-wise comparison of the components of two vectors for inequality.
   *
   * This method should not be used for floating-point valued vectors. Instead,
   * !equals() is the suitable comparison.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool operator!=(const Vector<Size, Scalar>& lhs, const Vector<Size, Scalar>& rhs);

  /**
   * Returns the index of the first component pair that differs.
   */
  template <int Size, typename Scalar>
  INLINE_METHOD int equalsReturnIndex(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );

  template <int Size>
  INLINE_METHOD GPU_CALLABLE_METHOD int equalsReturnIndex(const Vector<Size, int>& lhs, const Vector<Size, int>& rhs);

  /**
   * Run through both vectors and count how many entries are the same.
   */
  template <int Size>
  INLINE_METHOD GPU_CALLABLE_METHOD int countEqualEntries(const Vector<Size, int>& lhs, const Vector<Size, int>& rhs);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int countEqualEntries(
    const Vector<Size, Scalar>& lhs,
    const Vector<Size, Scalar>& rhs,
    const Scalar                tolerance = NUMERICAL_ZERO_DIFFERENCE
  );
} // namespace tarch::la

#include "tarch/la/VectorVectorOperations.cpph"
