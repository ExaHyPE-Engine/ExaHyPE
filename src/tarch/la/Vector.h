// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "la.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <string>
#include <bitset>
#include <sstream>
#include <initializer_list>

namespace tarch {
  namespace la {
    template <int Size, typename Scalar>
    struct Vector;

    template <typename NewScalarType, int Size, typename Scalar>
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
 *
 * ## GPGPU offloading
 *
 * ### OpenMP
 *
 * To make the vector operations available on the GPU, they have to be labelled
 * as offloadable according to Section 4.5.2 (page 117) of the standard
 * (https://www.openmp.org/wp-content/uploads/openmp-examples-4.5.0.pdf).
 * There, the text highlights that declare target is a declaration thing, i.e.,
 * should not be applied to the definition.
 *
 * However, it is not clear whether the compiler expects the
 * function definition to be within the declaration, too, and if this makes a
 * difference here at all: As the vector is a template, it is header-only
 * anyway. Therefore, the compiler implicitly should have the definition at
 * hand and should be able to identify that code for the GPU has to be
 * generated, too. We only have to be sure that no explicit instantiation of
 * a vector ignores this context, i.e. do not use explicit instantiation if you
 * employ GPUs (through OpenMP).
 *
 * With GCC, I struggled using the Vector's copy constructors. That is,
 * my impression is that the GCC compiler struggles to handle templates plus
 * overloading if these are implicitly used within a map clause.
 * Therefore, I often offload code through the standard constructor, deploy
 * the vector components manually, thus avoid all copy constructor
 * invocations explicitly, and finally repuzzle the vector together on the
 * GPU. This implies that GPU routines may never return
 * a Vector instance. See the Rusanov kernels for examples where this
 * leads to uglier code than necessary. But it works.
 *
 *
 * ## SYCL
 *
 * Our SYCL kernels seem to be fine without explicit support of the vector,
 * but this is mainly due to the fact that SYCL offers its own range class,
 * and we construct our vectors typically from this range class. The other
 * application domain is the double vectors within the cell descriptors,
 * but we map those onto specialised variants before we offload anyway, so
 * they are on the GPU prior to the kernel launch. However, when we run our
 * parallel for loops with SYCL on the host, then we also need to make sure
 * that the vector class is device copyable (i.e. trivially copyable). We
 * mark our copy assignment operator and copy constructor as default to let
 * the compiler generated these for us. As the class contains no dynamic
 * memory, we can safely do so: https://en.cppreference.com/w/cpp/language/classes#Trivially_copyable_class
 *
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
template <int Size, typename Scalar>
struct tarch::la::Vector {
  private:
    Scalar _values[Size];

  public:
    /**
     * Clang requires the always_inline attribute, as it otherwise makes weird decisions.
     */
    inline Vector() InlineMethod = default;

    inline GPUCallableMethod
    Vector(const Scalar* values) InlineMethod;

    /**
     * Initialisation via initialisation list.
     *
     * You can't inline an initialisation list, as the realisation of this
     * routine relies on an iterator and the iterator within the C++ std lib
     * won't be inlined.
     */
    inline GPUCallableMethod
    Vector(std::initializer_list<Scalar> values) InlineMethod;

    inline GPUCallableMethod
    Vector(const std::bitset<Size>& values) InlineMethod;

    /**
     * Construct new vector and initialise all components with initialValue.
     */
    inline GPUCallableMethod
    Vector(const Scalar& initialValue) InlineMethod;

    inline ~Vector() InlineMethod = default;

    /**
     * Assignment operator for any vector type.
     *
     * Default assignment as this class needs to be trivially copyable.
     */
    inline GPUCallableMethod
    Vector<Size, Scalar>& operator = (const Vector<Size, Scalar>& toAssign) InlineMethod = default;

    /**
     * Copy constructor to copy from any vector type.
     *
     * @see operator= for default copy.
     */
    inline GPUCallableMethod
    Vector(const Vector<Size, Scalar>& toCopy) InlineMethod = default;

    /**
     * Returns the number of components of the vector.
     */
    inline GPUCallableMethod
    int size() const InlineMethod;

    /**
     * Returns read-only ref. to component of given index.
     *
     * <h2> SSE Optimisation </h2>
     *
     * - We have to manually inline this operation. Otherwise, icc interprets operator
     *   calls, i.e. vector element accesses, as function calls and does not vectorise
     *   loops containing vector element accesses.
     */
    inline GPUCallableMethod
    const Scalar& operator[] (int index) const InlineMethod {
      #if defined(GPUOffloadingOff) and !defined(SharedSYCL)
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
    inline GPUCallableMethod
    Scalar& operator[] (int index) InlineMethod {
      #if defined(GPUOffloadingOff) and !defined(SharedSYCL)
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
    inline GPUCallableMethod
    const Scalar& operator() (int index) const InlineMethod {
      #if defined(GPUOffloadingOff) and !defined(SharedSYCL)
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
    inline GPUCallableMethod
    Scalar& operator() (int index) InlineMethod {
      #if defined(GPUOffloadingOff) and !defined(SharedSYCL)
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
    inline GPUCallableMethod
    Scalar* data() InlineMethod {
      return _values;
    }

    inline GPUCallableMethod
    const Scalar* data() const InlineMethod {
      return _values;
    }
};
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<1, int>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<2, int>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<1, double>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<2, double>>::value,
  "Vector should be trivially copyable!"
);

#if defined(SharedSYCL) or defined(GPUOffloadingSYCL)
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<1, int>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, int>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<1, double>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, double>>::value,
  "Vector should be SYCL device copyable!"
);
#endif

#include "tarch/la/Vector.cpph"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/la/VectorSlice.h"
