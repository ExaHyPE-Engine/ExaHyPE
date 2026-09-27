// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearisedExclusive(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                       max,
    int                                       dim
  );

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dDelinearised(int index, int max);

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dDelinearisedExclusive(
    int index,
    int max,
    int dim
  );

  template <typename Functor>
  INLINE_METHOD constexpr void dForEach(int max, const Functor& body);

  template <typename Functor>
  INLINE_METHOD constexpr void dForEach(const tarch::la::Vector<DIMENSIONS, int>& max, const Functor& body);

  template <typename Functor>
  INLINE_METHOD constexpr void dForEachCorner(const Functor& body);
} // namespace peano4

#include "peano4/Loop.cpph"
