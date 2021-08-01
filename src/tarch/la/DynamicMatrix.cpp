#include "DynamicMatrix.h"
#include "tarch/multicore/multicore.h"


// @todo raus
#include <iostream>

tarch::la::DynamicMatrix kroneckerProduct( const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs ) {
  return std::move( tarch::la::DynamicMatrix::kroneckerProduct(lhs,rhs) );
}


tarch::la::DynamicMatrix tarch::la::DynamicMatrix::kroneckerProduct( const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs ) {
  tarch::la::DynamicMatrix result(lhs._rows*rhs._rows, lhs._cols*rhs._cols);

  for (int lhsRow=0; lhsRow<lhs._rows; lhsRow++)
  for (int rhsRow=0; rhsRow<rhs._rows; rhsRow++)
  for (int lhsCol=0; lhsCol<lhs._cols; lhsCol++)
  for (int rhsCol=0; rhsCol<rhs._cols; rhsCol++) {
    result._m[ result.serialise(lhsRow*rhs._rows+rhsRow, lhsCol*rhs._cols+rhsCol) ]
      = lhs._m[ lhs.serialise(lhsRow,lhsCol) ] * rhs._m[ rhs.serialise(rhsRow,rhsCol) ];
  }

  return std::move(result);
}


tarch::la::DynamicMatrix::DynamicMatrix(int rows, int cols, std::initializer_list< std::initializer_list<double> > values):
  _cols(cols),
  _rows(rows),
  _m(nullptr) {
  _m = tarch::allocateMemory(rows*cols,tarch::MemoryLocation::Heap);

  int index = 0;
  for (typename std::initializer_list< std::initializer_list<double> >::const_iterator p  = values.begin(); p!=values.end(); p++)
  for (typename std::initializer_list<double>::const_iterator pp = p->begin(); pp!=p->end(); pp++) {
    _m[index] = *pp;
    index++;
  }
}


tarch::la::DynamicMatrix& tarch::la::DynamicMatrix::operator=( std::initializer_list< std::initializer_list<double> > values ) {
  int index = 0;
  for (typename std::initializer_list< std::initializer_list<double> >::const_iterator p  = values.begin(); p!=values.end(); p++)
  for (typename std::initializer_list<double>::const_iterator pp = p->begin(); pp!=p->end(); pp++) {
    _m[index] = *pp;
    index++;
    assertion(index<_cols*_rows);
  }
  return *this;
}



tarch::la::DynamicMatrix::DynamicMatrix(int rows, int cols):
  _cols(cols),
  _rows(rows),
  _m(nullptr) {
  _m = tarch::allocateMemory(rows*cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,rows*cols,0.0);
}


tarch::la::DynamicMatrix::DynamicMatrix(const DynamicMatrix&& value):
  _cols(value._cols),
  _rows(value._rows),
  _m(value._m) {
}

tarch::la::DynamicMatrix::~DynamicMatrix() {
  tarch::freeMemory(_m,tarch::MemoryLocation::Heap);
}



bool tarch::la::DynamicMatrix::operator==(const DynamicMatrix& matrix) const {
  bool result = (_rows==matrix._rows and _cols==matrix._cols);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    result &= tarch::la::equals( _m[ serialise(row,col) ], matrix._m[ serialise(row,col) ] );
  }

  return result;
}


/*
tarch::la::DynamicMatrix& tarch::la::DynamicMatrix::operator=( double** values ) {
 for (int col=0; col<_cols; col++)
 for (int row=0; row<_rows; row++) {
   _x[col*_rows+row] = values[col][row];
 }
 return *this;
}


tarch::la::DynamicMatrix& tarch::la::DynamicMatrix::operator=( double* values ) {
 for (int col=0; col<_cols; col++)
 for (int row=0; row<_rows; row++) {
   _x[col*_rows+row] = values[col*_cols+row];
 }
 return *this;
}
*/


double& tarch::la::DynamicMatrix::operator()(int row, int col) {
  assertion(row>=0);
  assertion(col>=0);
  assertion(row<_rows);
  assertion(col<_cols);
  return _m[ serialise(row,col) ];
}


double  tarch::la::DynamicMatrix::operator()(int row, int col) const {
  assertion(row>=0);
  assertion(col>=0);
  assertion(row<_rows);
  assertion(col<_cols);
  return _m[ serialise(row,col) ];
}


void tarch::la::DynamicMatrix::multiply( double* result, double* x ) {
  std::fill_n(result,_rows,0.0);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    result[row] += _m[ serialise(row,col) ] * x[row];
  }
}


std::string tarch::la::DynamicMatrix::toString() const {
  std::ostringstream msg;
  msg << "(rows=" << _rows << ",cols=" << _cols << ",{";
  for (int row=0; row<_rows; row++) {
    if (row!=0) msg << ",";
    msg << "{";
    for (int col=0; col<_cols; col++) {
      if (col!=0) msg << ",";
      msg << _m[serialise(row,col)];
    }
    msg << "}";
  }
  msg << "})";
  return msg.str();
}


void tarch::la::DynamicMatrix::batchedMultiplyAoS( double* result, double* x, int batchSize, int resultSize, int firstRow  ) {
  assertion3( batchSize>0,  batchSize, resultSize, firstRow );
  assertion3( resultSize>0, batchSize, resultSize, firstRow );
  assertion3( firstRow>=0,  batchSize, resultSize, firstRow );

  std::fill_n(result,resultSize*batchSize,0.0);

  for (int i=0; i<batchSize; i++)
  for (int col=0; col<_cols; col++)
  for (int rowInResult=0; rowInResult<resultSize; rowInResult++) {
    assertion3(rowInResult + firstRow<_rows,  batchSize, resultSize, firstRow);
    result[rowInResult*batchSize+i] += _m[ serialise(rowInResult + firstRow,col) ] * x[col*batchSize+i];
  }
}


void tarch::la::DynamicMatrix::replicateRows( int numberOfReplications, int shiftAfterEveryReplication ) {
  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  assertion2(numberOfReplications>=1,numberOfReplications,shiftAfterEveryReplication);
  assertion2(shiftAfterEveryReplication>=0,numberOfReplications,shiftAfterEveryReplication);

  _rows = _rows*numberOfReplications;
  _cols = _cols+shiftAfterEveryReplication*(numberOfReplications-1);

  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);

  std::fill_n(_m,_rows*_cols,0.0);

  for (int i=0; i<numberOfReplications; i++)
  for (int col=0; col<oldCols; col++)
  for (int row=0; row<oldRows; row++) {
    _m[ serialise(row+i*oldRows,col+i*shiftAfterEveryReplication) ] = oldData[ serialise(row,col,oldRows,oldCols) ];
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


void tarch::la::DynamicMatrix::insertColumns( int number, int where, int repeatEveryKColumns ) {
  assertion4( number>0,    number, where, repeatEveryKColumns, toString() );
  assertion4( where>=0,    number, where, repeatEveryKColumns, toString() );
  assertion4( where<=_cols, number, where, repeatEveryKColumns, toString() );
  assertion4( repeatEveryKColumns>=0, number, where, repeatEveryKColumns, toString() );

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  const int numberOfInsertions = repeatEveryKColumns==0 ? number : (_cols/repeatEveryKColumns) * number;
  _cols+=numberOfInsertions;

  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,_rows*_cols,0.0);

  int imageColumn = 0;
  for (int col=0; col<oldCols; col++) {
    if (col==where) {
      imageColumn += number;
      if (repeatEveryKColumns>=1) {
        where += repeatEveryKColumns;
      }
    }
    for (int row=0; row<oldRows; row++) {
      _m[ serialise(row,imageColumn) ] = oldData[ serialise(row,col,oldRows,oldCols) ];
    }
    imageColumn++;
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


void tarch::la::DynamicMatrix::insertRows( int number, int where, int repeatEveryKRows ) {
  assertion4( number>0,    number, where, repeatEveryKRows, toString() );
  assertion4( where>=0,    number, where, repeatEveryKRows, toString() );
  assertion4( where<_cols, number, where, repeatEveryKRows, toString() );
  assertion4( repeatEveryKRows>=0, number, where, repeatEveryKRows, toString() );

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  const int numberOfInsertions = repeatEveryKRows==0 ? number : (_rows/repeatEveryKRows) * number;
  _rows+=numberOfInsertions;

  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,_rows*_cols,0.0);

  int imageRow = 0;
  for (int row=0; row<oldRows; row++) {
    if (row==where) {
      imageRow += number;
      if (repeatEveryKRows>=1) {
        where += repeatEveryKRows;
      }
    }
    for (int col=0; col<oldCols; col++) {
      _m[ serialise(imageRow,col) ] = oldData[ serialise(row,col,oldRows,oldCols) ];
    }
    imageRow++;
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


void tarch::la::DynamicMatrix::removeColumn( int number ) {
  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;
  _cols--;


  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,_rows*_cols,0.0);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    int preImageColumn = col<number ? col : col+1;
    _m[ serialise(row,col) ] = oldData[ serialise(row,preImageColumn,oldRows,oldCols) ];
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}
