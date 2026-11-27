// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

TARGET_DECLARE
template <typename Scalar>
struct tarch::la::Vector<4, Scalar> {
  Scalar _values[4]{};

  Vector() = default;
  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar* values);
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr Vector(std::initializer_list<Scalar> values);
  INLINE_METHOD Vector(const std::bitset<4>& values);
  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar& initialValue);

  INLINE_METHOD GPU_CALLABLE_METHOD int getSize() const;
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator[](int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator[](int index);
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int index);

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* getData();
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* getData() const;
};
TARGET_DECLARE_END

#include "tarch/la/Vector4.cpph"
