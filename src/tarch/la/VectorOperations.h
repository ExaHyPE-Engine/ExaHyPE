// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "ScalarOperations.h"
#include "Vector.h"

#include <cmath>
#include <iosfwd>
#include <vector>

template <int Size, typename Scalar>
std::string toString(const tarch::la::Vector<Size, Scalar>& vector);

namespace tarch::la {
  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm1(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm1(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm1(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm1(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm2(const Vector<Size, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm2Squared(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm2Squared(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm2Squared(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar norm2Squared(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar normMax(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar normMax(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar normMax(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar normMax(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> abs(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> abs(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> abs(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> abs(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar sum(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar volume(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar volume(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar volume(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar volume(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allEntriesAreTheSame(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allEntriesAreTheSame(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allEntriesAreTheSame(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool allEntriesAreTheSame(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar average(const Vector<Size, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMax(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMax(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMax(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMax(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMin(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMin(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMin(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int indexMin(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar max(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar max(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar max(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar max(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar maxAbs(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar maxAbs(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar maxAbs(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar maxAbs(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar min(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar min(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar min(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar min(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> invertEntries(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> invertEntries(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> invertEntries(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> invertEntries(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool contains(const Vector<Size, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool contains(const Vector<2, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool contains(const Vector<3, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD bool contains(const Vector<4, Scalar>& vector, const Scalar& value);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int count(const Vector<Size, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int count(const Vector<2, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int count(const Vector<3, Scalar>& vector, const Scalar& value);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int count(const Vector<4, Scalar>& vector, const Scalar& value);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> floor(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<2, Scalar> floor(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<3, Scalar> floor(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<4, Scalar> floor(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryNan(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryNan(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryNan(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryNan(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryFinite(const Vector<Size, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryFinite(const Vector<2, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryFinite(const Vector<3, Scalar>& vector);
  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD int isEntryFinite(const Vector<4, Scalar>& vector);

  template <int Size, typename Scalar>
  INLINE_METHOD std::vector<Scalar> toSTLVector(const Vector<Size, Scalar>& vector);
} // namespace tarch::la

/**
 * Streams the component values into a comma separated representation.
 */
template <int Size, typename Scalar>
INLINE_METHOD std::ostream& operator<<(std::ostream& os, const tarch::la::Vector<Size, Scalar>& vector);

#include "VectorOperations.cpph"
