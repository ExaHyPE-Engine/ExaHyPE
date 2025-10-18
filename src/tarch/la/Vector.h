// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "la.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/CompilerSpecificSettings.h"

#include <bitset>
#include <initializer_list>
#include <sstream>
#include <string>

namespace tarch {
  namespace la {
    template <int Size, typename Scalar> struct Vector;

    /**
     * Convert the scalar type underlying a vector
     *
     * This routine takes a vector of size Size over type Scalar and returns a
     * new vector of the same size but with type NewScalarType. A typical usage
     * looks similar to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * tarch::la::Vector< DIMENSIONS, int > myVector = ...
     * tarch::la::Vector< DIMENSIONS, double > converted = tarch::la::convertScalar<double>( myVector );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * The conversion assumes that C++ natively supports a conversion from
     * Scalar to NewScalarType. We do not employ any explicit typecasting under
     * the hood.
     */
    template <typename NewScalarType, int Size, typename Scalar>
    inline GPU_CALLABLE_METHOD
    tarch::la::Vector<Size,NewScalarType> convertScalar(const tarch::la::Vector<Size,Scalar>& vector);
  }
}

/**
 * Pipes the elements of a vector into a std::string and returns the string.
 *
 * Not a member of the class as I otherwise can't translate it for GPUs.
 */
template <int Size, typename Scalar>
std::string toString(const tarch::la::Vector<Size,Scalar>& vector);

/**
 * Simple vector class
 *
 * Most features concerning vectors are deployed into separate headers:
 *
 * - Access only few elements within vector (subvector): VectorSlice.h
 * - Functions over the vector such as norms: VectorOperations.h
 * - Binary operators over vectors: VectorScalarOperations.h and
 *   VectorVectorOperations.h
 *
 * There are multiple ways how to construct a vector:
 *
 * - You can use the standard constructor which gives a vector with garbage
 *   entries. You can then set individual entries manually.
 * - You can use the constructor with a scalar which initialises all entries
 *   with the same value.
 * - You can use initialiser lists, i.e. lists in curly brackets { and }, to
 *   construct a vector from a list.
 *
 * To access entries of a vector, the class provides both access via an
 * array notation, i.e. through [...], and with round brackets. The latter
 * notation resembles Matlab. No matter which accessor you use, enumeration
 * always starts with zero.
 *
 * The class wraps an array of fixed size. Therefore, you can access the raw
 * data via data(), but this violates ideas of object-oriented programming
 * and hence should be done with care. Peano for example does this if and
 * only if it has to hand over raw pointers to MPI, e.g.
 *
 */
template <int Size, typename Scalar> struct tarch::la::Vector {
private:
  Scalar _values[Size];

  public:
    /**
     * Clang requires the always_inline attribute, as it otherwise makes weird decisions.
     */
    inline Vector() = default;

    inline GPU_CALLABLE_METHOD
    Vector(const Scalar* values);

    /**
     * Initialisation via initialisation list.
     *
     * You can't inline an initialisation list, as the realisation of this
     * routine relies on an iterator and the iterator within the C++ std lib
     * won't be inlined.
     */
    inline GPU_CALLABLE_METHOD
    Vector(std::initializer_list<Scalar> values);

    inline GPU_CALLABLE_METHOD
    Vector(const std::bitset<Size>& values);

    /**
     * Construct new vector and initialise all components with initialValue.
     */
    inline GPU_CALLABLE_METHOD
    Vector(const Scalar& initialValue);

    inline ~Vector() = default;

    /**
     * Assignment operator for any vector type.
     *
     * Default assignment as this class needs to be trivially copyable.
     */
    inline GPU_CALLABLE_METHOD
    Vector<Size, Scalar>& operator = (const Vector<Size, Scalar>& toAssign) = default;

    /**
     * Copy constructor to copy from any vector type.
     *
     * @see operator= for default copy.
     */
    inline GPU_CALLABLE_METHOD
    Vector(const Vector<Size, Scalar>& toCopy) = default;

    /**
     * Returns the number of components of the vector.
     */
    inline GPU_CALLABLE_METHOD
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
    inline GPU_CALLABLE_METHOD
    const Scalar& operator[] (int index) const {
      #if !defined(WITH_GPU)
      assertion3(index >= 0, index, Size, ::toString(*this));
      assertion3(index < Size, index, Size, ::toString(*this));
      #endif
      return _values[index];
    }

    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline GPU_CALLABLE_METHOD
    Scalar& operator[] (int index) {
      #if !defined(WITH_GPU)
      assertion3(index >= 0, index, Size, ::toString(*this));
      assertion3(index < Size, index, Size, ::toString(*this));
      #endif
      return _values[index];
    }

    /**
     * Returns read-only ref. to component of given index.
     *
     * If we use the vector on the GPU, we cannot have assertions. If we use
     * SYCL on the CPU for the multitasking, we cannot have assertions in the
     * SYCL part either, as SYCL aims to be platform-independent, i.e. has to
     * assume that the code generated also will be deployed to a GPU. See
     * discussion on @ref tarch_accelerator_SYCL.
     *
     * @see operator[] for remarks on SSE
     */
    inline GPU_CALLABLE_METHOD
    const Scalar& operator() (int index) const {
      #if !defined(WITH_GPU)
      assertion3(index >= 0, index, Size, ::toString(*this));
      assertion3(index < Size, index, Size, ::toString(*this));
      #endif
      return _values[index];
    }

    /**
     * Returns ref. to component of given index.
     *
     * @see operator[] for remarks on SSE
     */
    inline GPU_CALLABLE_METHOD
    Scalar& operator() (int index) {
      #if !defined(WITH_GPU)
      assertion3(index >= 0, index, Size, ::toString(*this));
      assertion3(index < Size, index, Size, ::toString(*this));
      #endif
      return _values[index];
    }

    /**
     * This routine returns a pointer to the first data element. Not a
     * beautiful one as it harms the OO idea, but in many cases it is
     * convenient to have this operation.
     */
    inline GPU_CALLABLE_METHOD
    Scalar* data() {
      return _values;
    }

    inline GPU_CALLABLE_METHOD
    const Scalar* data() const {
      return _values;
    }
};

#include "Vector.cpph"
#include "VectorOperations.h"
#include "VectorScalarOperations.h"
#include "VectorVectorOperations.h"
#include "VectorSlice.h"
