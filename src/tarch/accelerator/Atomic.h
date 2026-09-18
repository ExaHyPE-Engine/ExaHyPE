// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include <sycl/sycl.hpp>
#endif

TARGET_DECLARE
namespace tarch::accelerator {
  /**
   * Unsigned integer type of the given width, which a compare-and-swap on a
   * floating-point value reads and writes.
   */
  template <int Bytes>
  struct UnsignedOfWidth;

  template <>
  struct UnsignedOfWidth<4> final {
    using Type = unsigned int;
  };

  template <>
  struct UnsignedOfWidth<8> final {
    using Type = unsigned long long;
  };

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD T loadAtomic(const T& target);

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void storeAtomic(T& target, T value);

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void updateAtomicMax(T& target, T value);

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void updateAtomicMaxThroughCompareAndSwap(T& target, T value);
} // namespace tarch::accelerator
TARGET_DECLARE_END

namespace tarch::accelerator {
  template <typename T>
  INLINE_METHOD void updateAtomicMin(T& target, T value);

  template <typename T>
  INLINE_METHOD T incrementAtomic(T& target);

  template <typename T>
  INLINE_METHOD T addAtomic(T& target, T value);
} // namespace tarch::accelerator

#include "tarch/accelerator/Atomic.cpph"
