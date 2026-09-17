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
  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD T loadAtomic(const T& target);

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void storeAtomic(T& target, const T value);

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void updateAtomicMax(T& target, const T value);
} // namespace tarch::accelerator
TARGET_DECLARE_END

namespace tarch::accelerator {
  template <typename T>
  INLINE_METHOD void updateAtomicMin(T& target, const T value);

  template <typename T>
  INLINE_METHOD T incrementAtomic(T& target);

  template <typename T>
  INLINE_METHOD T addAtomic(T& target, const T value);
} // namespace tarch::accelerator

#include "tarch/accelerator/Atomic.cpph"
