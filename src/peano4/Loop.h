// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  /**
   * One bit per axis, giving the direction zForEach and zForEach3 run that axis in.
   */
  using LoopDirection = std::bitset<DIMENSIONS>;

  /**
   * @return a vector whose entries are all zero.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector();

  /**
   * @return a vector whose entries are all zero besides the dim-th one, which
   *         holds value.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector(int dim, int value);

  /**
   * @return a start vector whose entry per axis is 0 where direction holds that
   *         axis's bit and max-1 where it does not.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector(int max, const LoopDirection& direction);

  /**
   * Increments the first entry of counter, and carries over into the next entry
   * once an entry passes max-1.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * Increments the first entry of counter, and carries over into the next entry
   * once an entry passes that entry's own max-1.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(
    tarch::la::Vector<DIMENSIONS, int>&       counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  /**
   * Increments counter like dIncrement(counter, max) and leaves the entry
   * doNotExamine untouched.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(
    tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                 max,
    int                                 doNotExamine
  );

  /**
   * Increments an entry of counter where direction holds that entry's bit and
   * decrements it where it does not. The z-loop drives its counter with this.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(
    tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                 max,
    LoopDirection&                      direction
  );

  /**
   * @return true if all entries of counter are smaller than max.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCompare(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * @return true if all entries of counter are smaller than their corresponding
   *         entries in max.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCompare(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  /**
   * @return true if all entries of counter besides the dim-th one are smaller
   *         than max.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCompareExclusive(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                       max,
    int                                       dim
  );

  /**
   * @return the linearisation of counter, where entry k is multiplied by max^k
   *         and the products are accumulated. Entry 0 runs fastest.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * Invokes body(counter) for every point of [0,max)^d, entry 0 running
   * fastest.
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEach(int max, const Functor& body);

  /**
   * Per-axis-range variant of dForEach in which axis d runs over [0,max(d)).
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEach(const tarch::la::Vector<DIMENSIONS, int>& max, const Functor& body);

  /**
   * Exclusive loop that pins dimension dim to value and runs every other axis
   * over [0,max).
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachExclusive(int max, int dim, int value, const Functor& body);

  /**
   * Invokes body for each of the 2^d cube corners with the corner and its
   * linearised index.
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachCorner(const Functor& body);

  /**
   * Invokes body for each point of the 3^d stencil in lexicographic order with
   * the point and its linearised index.
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachStencil(const Functor& body);

  /**
   * D-dimensional space-filling (Peano-curve) loop over [0,max)^d for the given
   * initial direction. The direction is taken by value, so the caller's
   * LoopDirection is left unchanged.
   */
  template <typename Functor>
  INLINE_METHOD void zForEach(int max, LoopDirection direction, const Functor& body);

  /**
   * Space-filling loop for max = 3. For two and three dimensions it is driven by
   * a precomputed cell ordering; for higher dimensions it falls back to the
   * generic zForEach with max = 3.
   */
  template <typename Functor>
  INLINE_METHOD void zForEach3(LoopDirection direction, const Functor& body);
} // namespace peano4

#include "peano4/Loop.cpph"
