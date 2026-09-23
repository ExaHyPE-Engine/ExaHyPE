// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector();

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector(int dim, int value);

  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(
    tarch::la::Vector<DIMENSIONS, int>&       counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(
    tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                 max,
    int                                 doNotExamine
  );

  INLINE_METHOD GPU_CALLABLE_METHOD int dCompare(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  INLINE_METHOD GPU_CALLABLE_METHOD int dCompare(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  INLINE_METHOD GPU_CALLABLE_METHOD int dCompareExclusive(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                       max,
    int                                       dim
  );

  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEach(int max, const Functor& body);

  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEach(const tarch::la::Vector<DIMENSIONS, int>& max, const Functor& body);

  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachExclusive(int max, int dim, int value, const Functor& body);

  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachCorner(const Functor& body);

  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachStencil(const Functor& body);
} // namespace peano4

#include "peano4/Loop.cpph"
