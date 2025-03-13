// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/la.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <memory>
#include <string>
#include <bitset>
#include <sstream>
#include <initializer_list>

namespace tarch {
  namespace la {
    template <int Size, typename Scalar>
    struct SmartPointerVector;

    template <int Size, typename Scalar>
    struct Vector;

    template <typename NewScalarType, int Size, typename Scalar>
    tarch::la::SmartPointerVector<Size,NewScalarType> convertScalar(const tarch::la::SmartPointerVector<Size,Scalar>&  SmartPointerVector);
  }
}

/**
 * Pipes the elements of a SmartPointerVector into a std::string and returns the string.
 *
 * Not a member of the class as I otherwise can't translate it for GPUs.
 */
template<int Size, typename Scalar>
std::string toString( const tarch::la::SmartPointerVector<Size,Scalar>&  smartPointerVector );

/**
 * Simple SmartPointerVector class
 *
 * Mirrors exactly tarch::la::Vector, but internally uses a smart pointer
 * rather than a scalar. The class tries to minimise replication. Therefore,
 * it provides extensive conversion routines to and from a normal Vector but
 * limited functionality directly on the smart vector (where appropriate).
 * 
 */
template <int Size, typename Scalar>
struct tarch::la::SmartPointerVector {
  private:
    /**
     * In the original vector, this is
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Scalar _values[Size];
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    Scalar* _values;

    std::shared_ptr< Scalar[] > _dataSmartPointer;

  public:
    /**
     * Clang requires the always_inline attribute, as it otherwise makes weird decisions.
     */
    SmartPointerVector() InlineMethod;

    SmartPointerVector( const Scalar* values ) InlineMethod;

    /**
     * Initialisation via initialisation list.
     *
     * You can't inline an initialisation list, as the realisation of this
     * routine relies on an iterator and the iterator within the C++ std lib
     * won't be inlined.
     */
    SmartPointerVector( std::initializer_list<Scalar> values );

    SmartPointerVector( const std::bitset<Size>& values ) InlineMethod;

    /**
     * Construct new SmartPointerVector and initialize all components with initialValue.
     */
    SmartPointerVector(const Scalar& initialValue);

    /**
     * Copy constructor to copy from any SmartPointerVector type.
     *
     * The only way to accomplish this with enable-if is to specify a second
     * dummy argument with default value, which is (hopefully) optimized away.
     *
     * @see operator= for a discussion of SSE optimisation.
     */
    SmartPointerVector(const SmartPointerVector<Size,Scalar>&  toCopy);

    /**
     * Copy constructor
     *
     * As we are working with smart pointers, we do not realy copy under the
     * hood.
     *
     * I originally had
     *
     * ~~~~~~~~~~~~~~~~~~~~~~
  _dataSmartPointer( toCopy._dataSmartPointer ),
  _values( _dataSmartPointer.get() ) {
     * ~~~~~~~~~~~~~~~~~~~~~~
     *
     * but the the compiler told me that _dataSmartPointer is not initialised
     * at this point. Which I find weird. So now I initialise it explicitly.
     *
     *
     */
    SmartPointerVector(const Vector<Size,Scalar>&  toCopy);

    /**
     * Assignment operator for any SmartPointerVector type.
     *
     * <h2> SmartPointerVectorisation </h2>
     *
     * We do not allow assignment of a SmartPointerVector this itself. Consequently, we can
     * insert an ivdep statement and thus allow the compiler to optimise.
     */
    inline SmartPointerVector<Size,Scalar>& operator= (const SmartPointerVector<Size,Scalar>& toAssign) InlineMethod;

    /**
     * Conversion assignment operator
     */
    inline SmartPointerVector<Size,Scalar>& operator=( const Vector<Size,Scalar>& vector );

    /**
     * Assign all entries the same value
     *
     * As we work with smart pointers, we can return a copy here. Actually, I
     * got some compiler errors when I tried to return a reference.
     */
    inline SmartPointerVector<Size,Scalar>& operator=( const Scalar& vector );

    /**
     * Returns the number of components of the SmartPointerVector.
     */
    int size() const;

    /**
     * Returns read-only ref. to component of given index.
     *
     * <h2> SSE Optimisation </h2>
     *
     * - We have to manually inline this operation. Otherwise, icc interprets operator
     *   calls, i.e. SmartPointerVector element accesses, as function calls and does not SmartPointerVectorise
     *   loops containing SmartPointerVector element accesses.
     */
    inline const Scalar& operator[] (int index) const InlineMethod {
#if defined(GPUOffloadingOff)
          assertion3 ( index >= 0, index, Size, ::toString(*this) );
          assertion4 ( index < Size, index, Size, ::toString(*this), "you may not take the indexth entry from a SmartPointerVector with only Size components" );
#endif
        return _values[index];
    }

    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline Scalar& operator[] (int index) InlineMethod {
#if defined(GPUOffloadingOff)
          assertion3 ( index >= 0, index, Size, ::toString(*this) );
          assertion3 ( index < Size, index, Size, ::toString(*this) );
#endif
        return _values[index];
    }

    /**
     * Returns read-only ref. to component of given index.
     *
     * If we use the SmartPointerVector on the GPU, we cannot have assertions. If we use
     * SYCL on the CPU for the multitasking, we cannot have assertions in the
     * SYCL part either, as SYCL aims to be platform-independent, i.e. has to
     * assume that the code generated also will be deployed to a GPU. See
     * discussion on @ref tarch_accelerator_SYCL.
     *
     * @see operator[] for remarks on SSE
     */
    inline const Scalar& operator() (int index) const InlineMethod {
#if defined(GPUOffloadingOff)
          assertion3 ( index >= 0, index, Size, ::toString(*this) );
          assertion3 ( index < Size, index, Size, ::toString(*this) );
#endif
        return _values[index];
    }

    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline Scalar& operator() (int index) InlineMethod {
#if defined(GPUOffloadingOff)
        assertion3 ( index >= 0, index, Size, ::toString(*this) );
        assertion3 ( index < Size, index, Size, ::toString(*this) );
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

    const Scalar * data() const {
      return _values;
    }

    Vector<Size,Scalar> toVector() const;
};

#include "tarch/la/SmartPointerVector.cpph"

