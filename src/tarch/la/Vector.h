// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LA_VECTOR_H_
#define _TARCH_LA_VECTOR_H_



namespace tarch {
  namespace la {
    template<int Size, typename Scalar>
    struct Vector;

    template < typename NewScalarType, int Size, typename Scalar >
    tarch::la::Vector<Size,NewScalarType> convertScalar(const tarch::la::Vector<Size,Scalar>&  vector);
  }
}


#include "tarch/la/la.h"


#include <bitset>
#include <initializer_list>


/**
 * Simple vector class
 *
 * Most features concerning vectors are deployed into separate headers. Examples:
 *
 * - Access only few elements within vector (subvector): VectorSlice.h
 *
 *
 * <h2> GPGPU offloading </h2>
 *
 * The interesting part to read is Section 4.5.2 (page 117) of the standard:
 *
 * @see https://www.openmp.org/wp-content/uploads/openmp-examples-4.5.0.pdf
 *
 * declare target is a declaration thing, i.e. should not be applied to the 
 * definition. However, it is not clear whether the compiler expects the 
 * function definition to be within the declaration, too.
 *
 * Unfortunately, the standard does not explain how templates are handled
 * and whether we need the target wrapper around explicit instantiations, 
 * too. I interret the standard that the explicit declaration is the actual 
 * declaration and thus has to be wrapped. However, I did not manage to do
 * this.
 *
 * Also, I did never succeed in using the Vector's copy constructors. That is,
 * my impression is that the GCC compiler struggles to handle templates plus
 * overloading. The best thing I could do is to use the Vector within offloaded
 * code through the standard constructor and avoid all copy constructor 
 * invocations explicitly. This implies that GPU routines may never return 
 * a Vector instance. See the Rusanov.cpph kernels for examples where this 
 * leads to uglier code than necessary. Well, as long as it works ...
 *
 * 
 */
#pragma omp declare target
template<int Size, typename Scalar>
struct tarch::la::Vector {
  private:
    Scalar _values[Size];

  public:
    Vector();

    Vector( Scalar* values );

    Vector( std::initializer_list<Scalar> values );
    Vector( const std::bitset<Size>& values );

    /**
     * Construct new vector and initialize all components with initialValue.
     */
    Vector (const Scalar& initialValue);

    /**
     * Assignment operator for any vector type.
     *
     * <h2> Vectorisation </h2>
     *
     * We do not allow assignment of a vector this itself. Consequently, we can
     * insert an ivdep statement and thus allow the compiler to optimise.
     */
    Vector<Size,Scalar>& operator= (const Vector<Size,Scalar>& toAssign);

    /**
     * Copy constructor to copy from any vector type.
     *
     * The only way to accomplish this with enable-if is to specify a second
     * dummy argument with default value, which is (hopefully) optimized away.
     *
     * @see operator= for a discussion of SSE optimisation.
     */
    Vector(const Vector<Size,Scalar>&  toCopy);

    /**
     * Returns the number of components of the vector.
     */
    int size() const;

    /**
     * Returns read-only ref. to component of given index.
     *
     * <h2> SSE Optimisation </h2>
     *
     * - We have to manually inline this operation. Otherwise, icc interprets operator
     *   calls, i.e. vector element accesses, as function calls and does not vectorise
     *   loops containing vector element accesses.
     */
    inline const Scalar & operator[] (int index) const
      #ifdef UseManualInlining
      __attribute__((always_inline))
      #endif
      {
        #if !defined(GPUOffloading)
        assertion3 ( index >= 0, index, Size, toString() );
        assertion4 ( index < Size, index, Size, toString(), "you may not take the indexth entry from a vector with only Size components" );
	#endif
        return _values[index];
      }


    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline Scalar & operator[] (int index)
      #ifdef UseManualInlining
      __attribute__((always_inline))
      #endif
      {
        #if !defined(GPUOffloading)
        assertion3 ( index >= 0, index, Size, toString() );
        assertion3 ( index < Size, index, Size, toString() );
	#endif
        return _values[index];
      }

    /**
     * Returns read-only ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline const Scalar & operator() (int index) const
      #ifdef UseManualInlining
      __attribute__((always_inline))
      #endif
      {
        #if !defined(GPUOffloading)
        assertion3 ( index >= 0, index, Size, toString() );
        assertion3 ( index < Size, index, Size, toString() );
        #endif
        return _values[index];
      }

    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline Scalar & operator() (int index)
     #ifdef UseManualInlining
     __attribute__((always_inline))
     #endif
     {
       #if !defined(GPUOffloading)
       assertion3 ( index >= 0, index, Size, toString() );
       assertion3 ( index < Size, index, Size, toString() );
       #endif
       return _values[index];
     }

   
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


#include "tarch/la/Vector.cpph"


#pragma omp end declare target


/**
 * Explicit instantiation speeds up the translation process 
 * significantly. However, I did encounter numerous problems
 * whenever I tried to use it in combination with GPUs.
 */
#if !defined(GPUOffloading)
namespace tarch {
  namespace la {
    extern template
    struct tarch::la::Vector<2,double>;

    extern template
    struct tarch::la::Vector<3,double>;
  }
}
#endif


#include <sstream>

/**
 * Pipes the elements of a vector into a std::string and returns the string.
 *
 * Not a member of the class as I otherwise can't translate it for GPUs.
 */
template<int Size, typename Scalar>
std::string toString( const tarch::la::Vector<Size,Scalar>&  vector ) {
  std::ostringstream os;
  os << "[";
  for ( int i=0; i < Size; i++ ) {
    os << vector(i);
    if ( i + 1 < Size ) {
      os << ",";
    }
  }
  os << "]";
  return os.str();
}




#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/la/VectorSlice.h"


#endif

