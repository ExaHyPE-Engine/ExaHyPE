// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

#include <initializer_list>
#include <string>

namespace tarch::la {
  class DynamicMatrix;
} // namespace tarch::la

class tarch::la::DynamicMatrix {
private:
  int     _cols;
  int     _rows;
  double* _m;

  static INLINE_METHOD int serialise(int row, int col, [[maybe_unused]] int Rows, int Cols);

  INLINE_METHOD
  int serialise(int row, int col) const;

public:
  DynamicMatrix(int rows, int cols);

  ~DynamicMatrix();

  DynamicMatrix(int rows, int cols, std::initializer_list<std::initializer_list<double>> values);

  DynamicMatrix(const DynamicMatrix&);
  DynamicMatrix(DynamicMatrix&&);

  DynamicMatrix(const DynamicMatrix& lhs, const DynamicMatrix& rhs);

  DynamicMatrix& operator=(const DynamicMatrix&) = delete;

  std::string toString(bool addLineBreaks = false) const;

  INLINE_METHOD
  double* data();
  INLINE_METHOD
  const double* data() const;

  double& operator()(int row, int col);
  double  operator()(int row, int col) const;

  void scale(double value);

  void multiplyBySmallMatrix(double* result, const DynamicMatrix& matrix) const;

  /**
   * Apply the matrix to a batch of AoS vectors.
   */
  template <typename T = double>
  INLINE_METHOD void batchedMultiplyAoS(
    T* NOALIAS       result,
    const T* NOALIAS x,
    int              batchCount,
    int              resultSize,
    int              firstRow
  );

  template <typename T = double>
  INLINE_METHOD void batchedMultiplyAoS(T* NOALIAS result, const T* NOALIAS x, int batchCount);

  /**
   * Replicate row or column blocks with an optional shift between copies.
   */
  void replicateRows(
    int  blockSize,
    int  numberOfReplications,
    int  shiftAfterEveryReplication,
    bool extendColumnsToAccommodateShifts
  );
  void replicateCols(
    int  blockSize,
    int  numberOfReplications,
    int  shiftAfterEveryReplication,
    bool extendColumnsToAccommodateShifts
  );

  void insertEmptyColumns(int number, int where, int repeatEveryKColumns = 0);

  void removeColumn(int number);

  int rows() const;
  int cols() const;
};

namespace tarch::la {
  DynamicMatrix transpose(const DynamicMatrix& matrix);

  DynamicMatrix operator*(const DynamicMatrix& A, const DynamicMatrix& B);
} // namespace tarch::la

#include "DynamicMatrix.cpph"
