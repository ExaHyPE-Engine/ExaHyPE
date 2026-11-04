// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

TARGET_DECLARE
template <typename Scalar>
struct alignas(16) tarch::la::Matrix<4, 4, Scalar> {
public:
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix();
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix(const Scalar& value);
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix(std::initializer_list<Scalar> values);

  INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfRows() const;
  INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfColumns() const;
  INLINE_METHOD GPU_CALLABLE_METHOD int getSize() const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int rowIndex, int colIndex);
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int rowIndex, int colIndex) const;

  INLINE_METHOD std::string toString() const;
  INLINE_METHOD std::string toPrettyString(int numberOfDigits = 4) const;

  template <typename NewScalarType>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<4, 4, NewScalarType> convertScalar() const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* getData();
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* getData() const;

private:
  Scalar _values[16];
};
TARGET_DECLARE_END

#include "tarch/la/Matrix4x4.cpph"
