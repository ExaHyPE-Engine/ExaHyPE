// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LA_MATRIX_H_
#define _TARCH_LA_MATRIX_H_


namespace tarch {
  namespace la {
    template<int Rows, int Cols, typename Scalar>
    class Matrix;
  }
}


#include "tarch/la/MatrixOperations.h"
#include "tarch/la/MatrixVectorOperations.h"
#include "tarch/la/MatrixMatrixOperations.h"
#include "tarch/la/MatrixScalarOperations.h"

#include "tarch/compiler/CompilerSpecificSettings.h"


#include <string>
#include <initializer_list>



/**
 * Static (i.e. compiletime) sized matrix type.
 */
template<int Rows, int Cols, typename Scalar>
class tarch::la::Matrix {
  private:
    /**
     * Values of the matrix components.
     */
    Scalar _values[Rows*Cols];

  public:

  /**
   * Constructs a non-initialized matrix.
   */
  Matrix();
  /**
   * Constructs an initialized matrix.
   */
  Matrix( const Scalar&  value);
  Matrix( std::initializer_list<Scalar> values );

  /**
   * Returns the number of rows in the matrix.
   */
  int rows() const;

  /**
   * Returns the number of columns in the matrix.
   */
  int cols() const;

  /**
   * Returns the number of total elements in the matrix.
   */
  int size() const;

  /**
   * Returns element at given row and column index (from 0..size-1).
   *
   * @see Vector::operator[] for remarks on SSE efficiency
   */
  inline Scalar & operator() (
    int rowIndex,
    int colIndex
  )
    #ifdef UseManualInlining
    __attribute__((always_inline))
    #endif
    {
      assertion5( rowIndex >= 0, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( colIndex >= 0, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( rowIndex < Rows, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( colIndex < Cols, Rows, Cols, rowIndex, colIndex, toString() );
      return _values[rowIndex * Cols + colIndex];
    }

  /**
   * Returns const element at given row and column index (from 0..size-1).
   */
  inline const Scalar & operator() (
    int rowIndex,
    int colIndex
  ) const
    #ifdef UseManualInlining
    __attribute__((always_inline))
    #endif
    {
      assertion5( rowIndex >= 0, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( colIndex >= 0, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( rowIndex < Rows, Rows, Cols, rowIndex, colIndex, toString() );
      assertion5( colIndex < Cols, Rows, Cols, rowIndex, colIndex, toString() );
      return _values[rowIndex * Cols + colIndex];
    }

  std::string toString() const;

  std::string toPrettyString(int numberOfDigits=4) const;

  template <typename NewScalarType>
  tarch::la::Matrix<Rows,Cols,NewScalarType> convertScalar() const;

  /**
   * This routine returns a pointer to the first data element. Not a
   * beautiful one as it harms the OO idea, but in many cases it is
   * convenient to have this operation.
   */
  Scalar* data() {
    return _values;
  }

  const Scalar *  data() const {
    return _values;
  }
};



#include "tarch/la/Matrix.cpph"

#endif
