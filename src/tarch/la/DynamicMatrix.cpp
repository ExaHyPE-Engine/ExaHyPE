#include "DynamicMatrix.h"
#include "tarch/multicore/multicore.h"


#include <iostream>
#include <cstring>



tarch::la::DynamicMatrix kroneckerProduct( const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs ) {
  return tarch::la::DynamicMatrix(lhs,rhs,false);
}


tarch::la::DynamicMatrix::DynamicMatrix(const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs, bool innerProduct) {
  if (innerProduct) {
    assertionMsg(false, "not implemented yet");
  }
  else {
    _rows = lhs._rows*rhs._rows;
    _cols = lhs._cols*rhs._cols;

    _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);

    for (int lhsRow=0; lhsRow<lhs._rows; lhsRow++)
    for (int rhsRow=0; rhsRow<rhs._rows; rhsRow++)
    for (int lhsCol=0; lhsCol<lhs._cols; lhsCol++)
    for (int rhsCol=0; rhsCol<rhs._cols; rhsCol++) {
      _m[ serialise(lhsRow*rhs._rows+rhsRow, lhsCol*rhs._cols+rhsCol) ]
        = lhs._m[ lhs.serialise(lhsRow,lhsCol) ] * rhs._m[ rhs.serialise(rhsRow,rhsCol) ];
    }
  }
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


tarch::la::DynamicMatrix::DynamicMatrix(const DynamicMatrix& rhs):
  _cols(rhs._cols),
  _rows(rhs._rows),
  _m(nullptr) {
  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  memcpy(_m, rhs._m, _cols*_rows*sizeof(double));
}


tarch::la::DynamicMatrix::DynamicMatrix(DynamicMatrix&& rhs):
  _cols(rhs._cols),
  _rows(rhs._rows),
  _m(rhs._m) {
  rhs._m = nullptr;
  rhs._cols = 0;
  rhs._rows = 0;
}


tarch::la::DynamicMatrix& tarch::la::DynamicMatrix::operator=( std::initializer_list< std::initializer_list<double> > values ) {
  int index = 0;
  for (typename std::initializer_list< std::initializer_list<double> >::const_iterator p  = values.begin(); p!=values.end(); p++)
  for (typename std::initializer_list<double>::const_iterator pp = p->begin(); pp!=p->end(); pp++) {
    assertion3(index<_cols*_rows, _cols, _rows, index);
    _m[index] = *pp;
    index++;
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


/*
tarch::la::DynamicMatrix::DynamicMatrix(const DynamicMatrix&& value):
  _cols(value._cols),
  _rows(value._rows),
  _m(value._m) {
}
*/


tarch::la::DynamicMatrix::~DynamicMatrix() {
  if (_m!=nullptr) {
    tarch::freeMemory(_m,tarch::MemoryLocation::Heap);
  }
}


bool tarch::la::DynamicMatrix::operator==(const DynamicMatrix& matrix) const {
  bool result = (_rows==matrix._rows and _cols==matrix._cols);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    result &= tarch::la::equals( _m[ serialise(row,col) ], matrix._m[ serialise(row,col) ] );
  }

  return result;
}


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


tarch::la::DynamicMatrix tarch::la::DynamicMatrix::id( int rows ) {
  tarch::la::DynamicMatrix result(rows,rows);
  for (int i=0; i<rows; i++) {
    result(i,i) = 1.0;
  }
  return result;
}


void tarch::la::DynamicMatrix::multiply( double* result, double* x ) {
  std::fill_n(result,_rows,0.0);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    result[row] += _m[ serialise(row,col) ] * x[row];
  }
}


std::string tarch::la::DynamicMatrix::toString(bool addLineBreaks) const {
  std::ostringstream msg;
  msg << "(rows=" << _rows << ",cols=" << _cols << ",{";
  if (addLineBreaks) msg << std::endl;
  for (int row=0; row<_rows; row++) {
    if (addLineBreaks)
      msg << std::endl;
    else {
      if (row!=0) msg << ",";
      msg << "{";
    }

    for (int col=0; col<_cols; col++) {
      if (addLineBreaks)
        msg << " ";
      else if (col!=0)
        msg << ",";
      msg << _m[serialise(row,col)];
    }

    if (not addLineBreaks)
      msg << "}";
  }
  if (addLineBreaks) msg << std::endl;
  msg << "})";
  return msg.str();
}


std::string tarch::la::DynamicMatrix::vectorToString( double* values, int entries, bool addLineBreaks ) {
  std::ostringstream msg;
  msg << "(entries=" << entries << ",{";
  for (int i=0; i<entries; i++) {
    if (addLineBreaks)
      msg << std::endl;
    else {
      if (i!=0) msg << ",";
      msg << values[i];
    }
  }
  msg << "})";
  return msg.str();
}


void tarch::la::DynamicMatrix::batchedMultiplyAoS( double* result, double* x, int batchCount ) {
  batchedMultiplyAoS(
    result, x,
	batchCount,
	_rows,
	0
  );
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


void tarch::la::DynamicMatrix::insertEmptyColumns( int number, int where, int repeatEveryKColumns ) {
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


void tarch::la::DynamicMatrix::insertEmptyRows( int number, int where, int repeatEveryKRows ) {
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


void tarch::la::DynamicMatrix::shiftRowsDown(int shift,bool wrap) {
  double* oldData = _m;
  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,_rows*_cols,0.0);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    int imageRow = row + shift;
    if (wrap) imageRow %= _rows;
    if (imageRow>=0 and imageRow<_rows) {
      _m[ serialise(imageRow,col) ] = oldData[ serialise(row,col) ];
    }
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


void tarch::la::DynamicMatrix::shiftColumnsRight(int shift,bool wrap) {
  double* oldData = _m;
  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);
  std::fill_n(_m,_rows*_cols,0.0);

  for (int col=0; col<_cols; col++)
  for (int row=0; row<_rows; row++) {
    int imageCol = col+ shift;
    if (wrap) imageCol %= _cols;
    if (imageCol>=0 and imageCol<_cols) {
      _m[ serialise(row,imageCol) ] = oldData[ serialise(row,col) ];
    }
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


int tarch::la::DynamicMatrix::rows() const {
  return _rows;
}


int tarch::la::DynamicMatrix::cols() const {
  return _cols;
}


void tarch::la::DynamicMatrix::replicateRows( int blockSize, int numberOfReplications, int shiftAfterEveryReplication, bool extendColumnsToAccommodateShifts ) {
  assertion3( blockSize>=1, blockSize, numberOfReplications, shiftAfterEveryReplication );
  assertion3( _rows%blockSize==0, blockSize, numberOfReplications, shiftAfterEveryReplication );
  assertion3( numberOfReplications>=2, blockSize, numberOfReplications, shiftAfterEveryReplication );

  double* oldData = _m;
  int     oldRows = _rows;
  int     oldCols = _cols;

  assertion2(numberOfReplications>=1,numberOfReplications,shiftAfterEveryReplication);
  assertion2(shiftAfterEveryReplication>=0,numberOfReplications,shiftAfterEveryReplication);

  const int numberOfBlocks = _rows / blockSize;
  _rows = _rows*numberOfReplications;
  if (extendColumnsToAccommodateShifts) {
    _cols = _cols+shiftAfterEveryReplication*(numberOfReplications-1);
  }

  _m = tarch::allocateMemory(_rows*_cols,tarch::MemoryLocation::Heap);

  std::fill_n(_m,_rows*_cols,0.0);

  for (int block=0; block<numberOfBlocks; block++) {
    for (int replication=0; replication<numberOfReplications; replication++) {
      for (int blockRow=0; blockRow<blockSize; blockRow++) {
        const int destRow = (block*numberOfReplications+replication)*blockSize+blockRow;
        const int srcRow  = block*blockSize+blockRow;
        for (int col=0; col<oldCols; col++) {
          const int destCol = col+replication*shiftAfterEveryReplication;
          if (destCol<_cols) {
            _m[ serialise( destRow,destCol) ] = oldData[ serialise(srcRow,col,oldRows,oldCols) ];
          }
        }
      }
    }
  }

  tarch::freeMemory(oldData,tarch::MemoryLocation::Heap);
}


tarch::la::DynamicMatrix operator*(const tarch::la::DynamicMatrix& A, const tarch::la::DynamicMatrix& B) {
  tarch::la::DynamicMatrix result(A.rows(),B.cols());

  assertionEquals( A.cols(), B.rows() );

  for (int row=0; row<A.rows(); row++)
  for (int col=0; col<B.cols(); col++)
  for (int i=0; i<A.cols(); i++) {
    result(row,col) += A(row,i) * B(i,col);
  }

  return result;
}
