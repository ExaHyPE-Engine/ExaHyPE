// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <iomanip>
#include <sstream>

template <typename Scalar>
struct alignas(8) tarch::la::Matrix<2, 2, Scalar> {
private:
  Scalar _values[4];

  friend Matrix<2, 2, Scalar> buildMatrix<2, 2, Scalar>(const Scalar* values);

public:
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix();

  INLINE_METHOD GPU_CALLABLE_METHOD Matrix(const Scalar& value);

  INLINE_METHOD
  Matrix(std::initializer_list<Scalar> values);

  INLINE_METHOD GPU_CALLABLE_METHOD int rows() const;

  INLINE_METHOD GPU_CALLABLE_METHOD int cols() const;

  INLINE_METHOD GPU_CALLABLE_METHOD int size() const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int rowIndex, int colIndex);

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int rowIndex, int colIndex) const;

  std::string toString() const;

  std::string toPrettyString(int numberOfDigits = 4) const;

  template <typename NewScalarType>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, NewScalarType> convertScalar() const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* data();

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* data() const;
};

#include "Matrix2x2.cpph"
