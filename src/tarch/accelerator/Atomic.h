// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::accelerator {
  template <typename T>
  INLINE_METHOD T updateAtomicMin(T& target, const T value);

  template <typename T>
  INLINE_METHOD T updateAtomicMax(T& target, const T value);

  template <typename T>
  INLINE_METHOD T incrementAtomic(T& target);

  template <typename T>
  INLINE_METHOD T addAtomic(T& target, const T value);

  template <typename T>
  INLINE_METHOD void storeAtomic(T& target, const T value);
} // namespace tarch::accelerator

#include "tarch/accelerator/Atomic.cpph"
