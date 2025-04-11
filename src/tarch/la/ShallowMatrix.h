// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace tarch {
  namespace la {
    template<int Rows, int Cols, typename Scalar>
    class ShallowMatrix;
  }
}


#include "tarch/la/ShallowMatrixVectorOperations.h"

#include "tarch/compiler/CompilerSpecificSettings.h"

#include <string>



/**
 * Shallow matrix wrapping an existing array
 *
 * You are in charge of handling the underlying memory, i.e. the class does
 * neither allocate anything nor destroy anything
 */
template<int Rows, int Cols, typename Scalar>
class tarch::la::ShallowMatrix {
  private:
    /**
     * Values of the matrix components.
     */
    Scalar* _values;

  public:
    /**
     * Constructs a non-initialized matrix.
     */
    ShallowMatrix(Scalar* values);

    /**
     * Shallow copy
     */
    ShallowMatrix(const ShallowMatrix& copy);

    /**
     * Shallow copy
     */
    ShallowMatrix& operator=(const ShallowMatrix& copy);

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

    /**
     * Convert the scalar type
     *
     * Can also be used to convert a shallow matrix into a proper one, i.e. to
     * deep copy.
     */
    template <typename NewScalarType>
    tarch::la::Matrix<Rows,Cols,NewScalarType> convertScalar() const;

    /**
     * Return pointer back to original data
     */
    Scalar* data() {
      return _values;
    }

    const Scalar *  data() const {
      return _values;
    }
};



#include "tarch/la/ShallowMatrix.cpph"

