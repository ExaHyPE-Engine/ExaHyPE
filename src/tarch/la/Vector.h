// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace tarch::la {
  template <int Size, typename Scalar>
  struct Vector;

  template <typename NewScalarType, int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Size, NewScalarType> convertScalar(
    const tarch::la::Vector<Size, Scalar>& vector
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar> makeVector(const Scalar& value0, const Scalar& value1);

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar> makeVector(
    const Scalar& value0,
    const Scalar& value1,
    const Scalar& value2
  );

  template <typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar> makeVector(
    const Scalar& value0,
    const Scalar& value1,
    const Scalar& value2,
    const Scalar& value3
  );
} // namespace tarch::la

template <int Size, typename Scalar>
INLINE_METHOD std::string toString(const tarch::la::Vector<Size, Scalar>& vector);

template <int Size, typename Scalar>
struct tarch::la::Vector {
public:
  Vector() = default;
  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar* values);
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr Vector(std::initializer_list<Scalar> values);
  INLINE_METHOD Vector(const std::bitset<Size>& values);
  INLINE_METHOD GPU_CALLABLE_METHOD Vector(const Scalar& initialValue);

  INLINE_METHOD GPU_CALLABLE_METHOD int getSize() const;
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator[](int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator[](int index);
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int index);

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* getData();
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* getData() const;

private:
  Scalar _values[Size]{};
};

template <typename Scalar>
INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, Scalar>
tarch::la::makeVector(const Scalar& value0, const Scalar& value1) {
  Vector<2, Scalar> result;
  result._values[0] = value0;
  result._values[1] = value1;
  return result;
}

template <typename Scalar>
INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Scalar>
tarch::la::makeVector(const Scalar& value0, const Scalar& value1, const Scalar& value2) {
  Vector<3, Scalar> result;
  result._values[0] = value0;
  result._values[1] = value1;
  result._values[2] = value2;
  return result;
}

template <typename Scalar>
INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<4, Scalar>
tarch::la::makeVector(const Scalar& value0, const Scalar& value1, const Scalar& value2, const Scalar& value3) {
  Vector<4, Scalar> result;
  result._values[0] = value0;
  result._values[1] = value1;
  result._values[2] = value2;
  result._values[3] = value3;
  return result;
}
TARGET_DECLARE_END

#include "tarch/la/Vector.cpph"
#include "tarch/la/Vector2.h"
#include "tarch/la/Vector3.h"
#include "tarch/la/Vector4.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"
