// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicMatrix.h"

#include "tarch/accelerator/accelerator.h"

#include <algorithm>
#include <cstring>
#include <sstream>

static_assert(
  sizeof(tarch::la::DynamicMatrix) <= 2 * sizeof(void*) + 2 * sizeof(int),
  "DynamicMatrix should stay a small handle"
);

tarch::la::DynamicMatrix::DynamicMatrix(const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs) {
  _rows = lhs._rows * rhs._rows;
  _cols = lhs._cols * rhs._cols;

  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);

  for (int lhsRow = 0; lhsRow < lhs._rows; lhsRow++)
    for (int rhsRow = 0; rhsRow < rhs._rows; rhsRow++)
      for (int lhsCol = 0; lhsCol < lhs._cols; lhsCol++)
        for (int rhsCol = 0; rhsCol < rhs._cols; rhsCol++) {
          _m[serialise(
            lhsRow * rhs._rows + rhsRow,
            lhsCol * rhs._cols + rhsCol
          )] = lhs._m[lhs.serialise(lhsRow, lhsCol)] * rhs._m[rhs.serialise(rhsRow, rhsCol)];
        }
}

tarch::la::DynamicMatrix::
  DynamicMatrix(int rows, int cols, std::initializer_list<std::initializer_list<double>> values):
  _cols(cols),
  _rows(rows),
  _m(nullptr) {
  _m = tarch::allocateMemory<double>(rows * cols, tarch::MemoryLocation::Heap);

  int index = 0;
  for (typename std::initializer_list<std::initializer_list<double>>::const_iterator p = values.begin();
       p != values.end();
       p++)
    for (typename std::initializer_list<double>::const_iterator pp = p->begin(); pp != p->end(); pp++) {
      _m[index] = *pp;
      index++;
    }
}

tarch::la::DynamicMatrix::DynamicMatrix(const DynamicMatrix& rhs):
  _cols(rhs._cols),
  _rows(rhs._rows),
  _m(nullptr) {
  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);
  memcpy(_m, rhs._m, _cols * _rows * sizeof(double));
}

tarch::la::DynamicMatrix::DynamicMatrix(DynamicMatrix&& rhs):
  _cols(rhs._cols),
  _rows(rhs._rows),
  _m(rhs._m) {
  rhs._m    = nullptr;
  rhs._cols = 0;
  rhs._rows = 0;
}

tarch::la::DynamicMatrix::DynamicMatrix(int rows, int cols):
  _cols(cols),
  _rows(rows),
  _m(nullptr) {
  _m = tarch::allocateMemory<double>(rows * cols, tarch::MemoryLocation::Heap);
  std::fill_n(_m, rows * cols, 0.0);
}

tarch::la::DynamicMatrix::~DynamicMatrix() {
  if (_m != nullptr) {
    tarch::freeMemory(_m, tarch::MemoryLocation::Heap);
  }
}

double& tarch::la::DynamicMatrix::operator()(int row, int col) {
  assertion4(row >= 0, row, col, _rows, _cols);
  assertion4(col >= 0, row, col, _rows, _cols);
  assertion4(row < _rows, row, col, _rows, _cols);
  assertion4(col < _cols, row, col, _rows, _cols);
  return _m[serialise(row, col)];
}

double tarch::la::DynamicMatrix::operator()(int row, int col) const {
  assertion(row >= 0);
  assertion(col >= 0);
  assertion(row < _rows);
  assertion(col < _cols);
  return _m[serialise(row, col)];
}

void tarch::la::DynamicMatrix::scale(double value) {
  for (int i = 0; i < _rows * _cols; i++) {
    _m[i] *= value;
  }
}

void tarch::la::DynamicMatrix::multiplyBySmallMatrix(double* result, const DynamicMatrix& matrix) const {
  std::fill_n(result, _rows * matrix.cols(), 0.0);

  for (int row = 0; row < _rows; row++) {
    for (int col = 0; col < matrix.cols(); col++) {
      for (int i = 0; i < _cols; i++) {
        result[row * matrix.cols() + col] += (*this)(row, i) * matrix(i, col);
      }
    }
  }
}

std::string tarch::la::DynamicMatrix::toString(bool addLineBreaks) const {
  std::ostringstream msg;
  msg << "(rows=" << _rows << ",cols=" << _cols << ",{";
  if (addLineBreaks)
    msg << std::endl;
  for (int row = 0; row < _rows; row++) {
    if (addLineBreaks)
      msg << std::endl;
    else {
      if (row != 0)
        msg << ",";
      msg << "{";
    }

    for (int col = 0; col < _cols; col++) {
      if (addLineBreaks)
        msg << " ";
      else if (col != 0)
        msg << ",";
      msg << _m[serialise(row, col)];
    }

    if (not addLineBreaks)
      msg << "}";
  }
  if (addLineBreaks)
    msg << std::endl;
  msg << "})";
  return msg.str();
}

void tarch::la::DynamicMatrix::insertEmptyColumns(int number, int where, int repeatEveryKColumns) {
  assertion4(number > 0, number, where, repeatEveryKColumns, toString());
  assertion4(where >= 0, number, where, repeatEveryKColumns, toString());
  assertion4(where <= _cols, number, where, repeatEveryKColumns, toString());
  assertion4(repeatEveryKColumns >= 0, number, where, repeatEveryKColumns, toString());

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  const int numberOfInsertions = repeatEveryKColumns == 0 ? number : (_cols / repeatEveryKColumns) * number;
  _cols += numberOfInsertions;

  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);
  std::fill_n(_m, _rows * _cols, 0.0);

  int imageColumn = 0;
  for (int col = 0; col < oldCols; col++) {
    if (col == where) {
      imageColumn += number;
      if (repeatEveryKColumns >= 1) {
        where += repeatEveryKColumns;
      }
    }
    for (int row = 0; row < oldRows; row++) {
      _m[serialise(row, imageColumn)] = oldData[serialise(row, col, oldRows, oldCols)];
    }
    imageColumn++;
  }

  tarch::freeMemory(oldData, tarch::MemoryLocation::Heap);
}

void tarch::la::DynamicMatrix::removeColumn(int number) {
  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;
  _cols--;

  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);
  std::fill_n(_m, _rows * _cols, 0.0);

  for (int col = 0; col < _cols; col++)
    for (int row = 0; row < _rows; row++) {
      int preImageColumn      = col < number ? col : col + 1;
      _m[serialise(row, col)] = oldData[serialise(row, preImageColumn, oldRows, oldCols)];
    }

  tarch::freeMemory(oldData, tarch::MemoryLocation::Heap);
}

int tarch::la::DynamicMatrix::rows() const { return _rows; }

int tarch::la::DynamicMatrix::cols() const { return _cols; }

tarch::la::DynamicMatrix tarch::la::transpose(const tarch::la::DynamicMatrix& matrix) {
  tarch::la::DynamicMatrix result(matrix.cols(), matrix.rows());
  for (int i = 0; i < matrix.rows(); i++) {
    for (int j = 0; j < matrix.cols(); j++) {
      result(j, i) += matrix(i, j);
    }
  }
  return result;
}

void tarch::la::DynamicMatrix::replicateRows(
  int  blockSize,
  int  numberOfReplications,
  int  shiftAfterEveryReplication,
  bool extendColumnsToAccommodateShifts
) {
  assertion3(blockSize >= 1, blockSize, numberOfReplications, shiftAfterEveryReplication);
  assertion3(_rows % blockSize == 0, blockSize, numberOfReplications, shiftAfterEveryReplication);
  assertion2(numberOfReplications >= 1, numberOfReplications, shiftAfterEveryReplication);
  assertion2(shiftAfterEveryReplication >= 0, numberOfReplications, shiftAfterEveryReplication);

  if (numberOfReplications == 1) {
    return;
  }

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  const int numberOfBlocks = _rows / blockSize;
  _rows                    = _rows * numberOfReplications;
  if (extendColumnsToAccommodateShifts) {
    _cols = _cols + shiftAfterEveryReplication * (numberOfReplications - 1);
  }

  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);

  std::fill_n(_m, _rows * _cols, 0.0);

  for (int block = 0; block < numberOfBlocks; block++) {
    for (int replication = 0; replication < numberOfReplications; replication++) {
      for (int blockRow = 0; blockRow < blockSize; blockRow++) {
        const int destRow = (block * numberOfReplications + replication) * blockSize + blockRow;
        const int srcRow  = block * blockSize + blockRow;
        for (int col = 0; col < oldCols; col++) {
          const int destCol = col + replication * shiftAfterEveryReplication;
          if (destCol < _cols) {
            _m[serialise(destRow, destCol)] = oldData[serialise(srcRow, col, oldRows, oldCols)];
          }
        }
      }
    }
  }

  tarch::freeMemory(oldData, tarch::MemoryLocation::Heap);
}

void tarch::la::DynamicMatrix::replicateCols(
  int  blockSize,
  int  numberOfReplications,
  int  shiftAfterEveryReplication,
  bool extendColumnsToAccommodateShifts
) {
  assertion3(blockSize >= 1, blockSize, numberOfReplications, shiftAfterEveryReplication);
  assertion3(_cols % blockSize == 0, blockSize, numberOfReplications, shiftAfterEveryReplication);
  assertion2(numberOfReplications >= 1, numberOfReplications, shiftAfterEveryReplication);
  assertion2(shiftAfterEveryReplication >= 0, numberOfReplications, shiftAfterEveryReplication);

  if (numberOfReplications == 1) {
    return;
  }

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  const int numberOfBlocks = _cols / blockSize;
  _cols                    = _cols * numberOfReplications;
  if (extendColumnsToAccommodateShifts) {
    _rows = _rows + shiftAfterEveryReplication * (numberOfReplications - 1);
  }

  _m = tarch::allocateMemory<double>(_rows * _cols, tarch::MemoryLocation::Heap);

  std::fill_n(_m, _rows * _cols, 0.0);

  for (int block = 0; block < numberOfBlocks; block++) {
    for (int replication = 0; replication < numberOfReplications; replication++) {
      for (int blockCol = 0; blockCol < blockSize; blockCol++) {
        const int destCol = (block * numberOfReplications + replication) * blockSize + blockCol;
        const int srcCol  = block * blockSize + blockCol;
        for (int row = 0; row < oldRows; row++) {
          const int destRow = row + replication * shiftAfterEveryReplication;
          if (destRow < _rows) {
            _m[serialise(destRow, destCol)] = oldData[serialise(row, srcCol, oldRows, oldCols)];
          }
        }
      }
    }
  }

  tarch::freeMemory(oldData, tarch::MemoryLocation::Heap);
}

tarch::la::DynamicMatrix tarch::la::operator*(const tarch::la::DynamicMatrix& A, const tarch::la::DynamicMatrix& B) {
  tarch::la::DynamicMatrix result(A.rows(), B.cols());

  assertionEquals(A.cols(), B.rows());

  for (int row = 0; row < A.rows(); row++)
    for (int col = 0; col < B.cols(); col++)
      for (int i = 0; i < A.cols(); i++) {
        result(row, col) += A(row, i) * B(i, col);
      }
  return result;
}
