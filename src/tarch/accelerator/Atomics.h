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
  INLINE_METHOD GPU_CALLABLE_METHOD int atomicLoad(const int* const target);

  INLINE_METHOD GPU_CALLABLE_METHOD void atomicStore(int* const target, const int value);

  INLINE_METHOD GPU_CALLABLE_METHOD void atomicMaximum(int* const target, const int value);
} // namespace tarch::accelerator
TARGET_DECLARE_END

#include "Atomics.cpph"
