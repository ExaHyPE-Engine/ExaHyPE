// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

template <typename Scalar>
struct alignas(8) tarch::la::Vector<2, Scalar> {
  Scalar _values[2]{};

  INLINE_METHOD GPU_CALLABLE_METHOD Vector();

  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar* values);

  INLINE_METHOD GPU_CALLABLE_METHOD Vector(std::initializer_list<Scalar> values);

  INLINE_METHOD
  Vector(const std::bitset<2>& values);

  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar& initialValue);

  INLINE_METHOD GPU_CALLABLE_METHOD int size() const;

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator[](int index) const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator[](int index);

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int index) const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int index);

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* data();

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* data() const;
};

#include "tarch/la/Vector2.cpph"
