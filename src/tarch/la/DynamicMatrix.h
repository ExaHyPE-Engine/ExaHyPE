// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LA_DYNAMIC_MATRIX_H_
#define _TARCH_LA_DYNAMIC_MATRIX_H_



#include "tarch/Assertions.h"
#include "ScalarOperations.h"

#include <string>


namespace tarch {
  namespace la {
    class DynamicMatrix;
  }
}




/**
 * My standard matrix is a matrix where the size is fixed at compile time.
 * This is a matrix class where the size is fixed at runtime.
 */
class tarch::la::DynamicMatrix {
  private:
    int       _cols;
    int       _rows;
    double*   _m;

    static int serialise( int row, int col, int Rows, int Cols ) {
      return col + row * Cols;
    }

    int serialise( int row, int col ) const {
      return serialise( row, col, _rows, _cols );
    }

  public:
    DynamicMatrix(int rows, int cols);
    ~DynamicMatrix();

    DynamicMatrix( int rows, int cols, std::initializer_list< std::initializer_list<double> > values );

    DynamicMatrix(const DynamicMatrix&) = delete;
    DynamicMatrix(const DynamicMatrix&&) = delete;

    DynamicMatrix(const DynamicMatrix& lhs, const DynamicMatrix& rhs, bool innerProduct);

    DynamicMatrix& operator=(const DynamicMatrix&) = delete;

    /**
     * This template allows you to write stuff like
     *
     *
     * <pre>

    double P[Rows][Cols] = {
      {3.0/3.0, 0.0,    0.0/3.0, 0.0,     0.0/3.0, 0.0,    0.0/3.0, 0.0},
      {    0.0, 0.0,        0.0, 0.0,         0.0, 0.0,        0.0, 0.0},
      {3.0/3.0, 0.0,    0.0/3.0, 0.0,     0.0/3.0, 0.0,    0.0/3.0, 0.0},
      {    0.0, 0.0,        0.0, 0.0,         0.0, 0.0,        0.0, 0.0},
      ...

    tarch::la::DynamicMatrix myP(Rows,Cols);
    myP = P;

       </pre>
     */
    DynamicMatrix& operator=( std::initializer_list< std::initializer_list<double> > values );
/*
    template <int Cols>
    DynamicMatrix& operator=( double values[][Cols] ) {
      assertionEquals(Cols,_cols);
      for (int col=0; col<_cols; col++)
      for (int row=0; row<_rows; row++) {
        _m[ serialise(row,col) ] = values[row][col];
      }
      return *this;
    }
*/


    template <int Cols>
    bool operator==(double values[][Cols]) const {
      bool result = Cols==_cols;
      for (int col=0; col<_cols; col++)
      for (int row=0; row<_rows; row++) {
        result &= tarch::la::equals( _m[ serialise(row,col) ], values[row][col]);
      }
      return result;
    }

    bool operator==(const DynamicMatrix& matrix) const;

    std::string toString() const;

    double& operator()(int row, int col);
    double  operator()(int row, int col) const;

    void multiply( double* result, double* x );

    /**
     * This operation assumes that x holds a whole batch of vectors in AoS format.
     * If batchSize is 5, then x points to five vectors of length _rows. The five
     * vectors are store as AoS, i.e. x[0] is the first entry of the first vector,
     * x[1] the first entry of the second vector, and so forth. The multiplication
     * applies the matrix to all vectors stored in x. It assumes that result also
     * holds a sequence of vectors.
     *
     * @param batchSize Number of elements within x/result to which the matrix has
     *                  to be applied in one rush.
     * @param resultSize Number of entries in vector result.  Please note that this
     *                  number will be multiplied with batchSize, i.e. it is the
     *                  entries per batch entry.
     * @param firstRow First row to pick from matrix. If the resultSize is equal to
     *                 the number of rows of the matrix, then this entry should be
     *                 0. Otherwise, you can pick only a few rows from the matrix
     *                 through this offset.
     */
    void batchedMultiplyAoS( double* result, double* x, int batchSize, int resultSize, int firstRow );

    void replicateRows( int numberOfReplications, int shiftAfterEveryReplication );
    /**
     * @param number How many columns shall be inserted. Has to be at least one.
     * @param where  Number of the first column to be inserted. Put in 0 to insert
     *               one to the left of the matrix.
     * @param repeatEveryKColumns If this one is zero, we insert only once. If it
     *               is bigger then zero, we insert number columns every
     *               repeatEveryKColumns starting from where. So if you pass in
     *               one, then every other column, we'll insert number of columns.
     */
    void insertColumns( int number, int where, int repeatEveryKColumns=0 );
    void insertRows( int number, int where, int repeatEveryKColumns=0 );

    void removeColumn( int number );

    static tarch::la::DynamicMatrix kroneckerProduct( const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs );
};


/**
 * Wrapper around static routine, so I don't have to use full-qualified name.
 */
tarch::la::DynamicMatrix kroneckerProduct( const tarch::la::DynamicMatrix& lhs, const tarch::la::DynamicMatrix& rhs );


#endif
