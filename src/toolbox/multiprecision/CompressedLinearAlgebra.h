// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_MULTIPRECISION_COMPRESSED_LINEAR_ALGEBRA_H_
#define _TOOLBOX_MULTIPRECISION_COMPRESSED_LINEAR_ALGEBRA_H_


#include "tarch/la/Vector.h"
#include "tarch/la/Matrix.h"


#include <vector>


namespace toolbox {
  /**
   * The heap namespace holds all routines that we use in connection with
   * Peano's heap data storage. This means
   *
   * - the actual heap classes
   * - allocators used for heaps and architectures requiring concurrent and aligned layouts
   * - data conversion/compression routines
   */
  namespace multiprecision {
    /**
     * Take a vector and compress it into a byte stream.
     *
     * <h2> Format of byte stream </h2>
     *
     * - The first entry is a number 1-7 which tells us how many bytes we
     *   invest per mantissa per entry. Let n be this number.
     * - We then (n+1)*Size entries logically with each entry hosting n+1
     *   bytes.
     * - The first byte per entry is the exponent.
     * - The remaining n bytes encode the mantissa.
     *
     * There's one reserved code: If the passed data holds zeros only, i.e. we
     * don't have to store any entry. So we add a zero as header an return.
     */
    template<int Size>
    std::vector<unsigned char> compress(const tarch::la::Vector<Size,double>& vector, double maxAbsoluteError);

    template<int Rows, int Cols>
    std::vector<unsigned char> compress(const tarch::la::Matrix<Rows,Cols,double>& matrix, double maxAbsoluteError);

    /**
     * Can't call it decompress only, as the result data type is not part of
     * the signature, i.e. I can't overload.
     */
    template<int Size>
    tarch::la::Vector<Size,double> uncompressVector( const std::vector<unsigned char>& stream );

    template<int Rows, int Cols>
    tarch::la::Matrix<Rows,Cols,double> uncompressMatrix( const std::vector<unsigned char>& stream );
  }
}


#include "CompressedLinearAlgebra.cpph"

#endif

