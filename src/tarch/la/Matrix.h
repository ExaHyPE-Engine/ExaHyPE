// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/ScalarOperations.h"

namespace tarch::la {
  template <int Rows, int Cols, typename Scalar>
  class Matrix;

  template <int Rows, int Cols, typename Scalar>
  INLINE_METHOD Matrix<Rows, Cols, Scalar> buildMatrix(const Scalar* values);
} // namespace tarch::la

template <int Rows, int Cols, typename Scalar>
class tarch::la::Matrix {
private:
  Scalar                            _values[Rows * Cols];
  friend Matrix<Rows, Cols, Scalar> buildMatrix<Rows, Cols, Scalar>(const Scalar* values);

public:
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix();
  INLINE_METHOD GPU_CALLABLE_METHOD Matrix(const Scalar& value);
  INLINE_METHOD GPU_CALLABLE_METHOD
  Matrix(std::initializer_list<Scalar> values);

  INLINE_METHOD GPU_CALLABLE_METHOD int           rows() const;
  INLINE_METHOD GPU_CALLABLE_METHOD int           cols() const;
  INLINE_METHOD GPU_CALLABLE_METHOD int           size() const;
  INLINE_METHOD GPU_CALLABLE_METHOD Scalar&       operator()(int rowIndex, int colIndex);
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int rowIndex, int colIndex) const;

  std::string toString() const;

  std::string toPrettyString(int numberOfDigits = 4) const;

  template <typename NewScalarType>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<Rows, Cols, NewScalarType> convertScalar() const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* data();

  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* data() const;
};

#include "tarch/la/Matrix.cpph"
#include "tarch/la/Matrix2x2.h"
#include "tarch/la/Matrix3x3.h"
#include "tarch/la/Matrix4x4.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixOperations.h"
#include "tarch/la/MatrixScalarOperations.h"
#include "tarch/la/MatrixVectorOperations.h"
