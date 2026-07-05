// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  /**
   * Is used by the z-loop. See zForEach and zForEach3.
   */
  using LoopDirection = std::bitset<DIMENSIONS>;

  /**
   * Construct start vector (0,0,....) for d-dimensional loop
   *
   * @return a vector containing zero values only.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector();

  /**
   * @return a vector containing only zero values besides the dim-th entry. This
   *         entry is set value.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector(int dim, int value);

  /**
   * Creates a start vector. Each component is set either 0 or max-1 depending
   * on direction: If direction is true, then the value 0 is zero. Used to seed
   * the generic zForEach z-loop.
   *
   * @return a start vector for an oscillating loop.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> dStartVector(int max, const LoopDirection& direction);

  /**
   * d-dimensional counterpart of increment operator
   *
   * This operation performs a d-dimensional increment on a given integer vector:
   * The first component of the vector is incremented. If the first component is
   * greater than max-1, the component is set zero and the next component is
   * incremented by one. This operation is used often by d-dimensional for-loops.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * d-dimensional counterpart of increment, where individual vector components have different max values
   *
   * This operation performs a d-dimensional increment on a given integer vector:
   * The first component of the vector is incremented. If the first component is
   * greater than max(0)-1, the component is set zero and the next component is
   * incremented by one. This operation is used often by d-dimensional for-loops.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void
  dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, const tarch::la::Vector<DIMENSIONS, int>& max);

  /**
   * Same operation as dIncrement(tarch::la::Vector<DIMENSIONS,int>,int), but now one dimension is not taken
   * into consideration. This is the increment behind the dForEachExclusive loop.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void
  dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, int max, int doNotExamine);

  /**
   * Operation similar to dIncrement, but is given a direction bitset that identifies
   * whether the counters has to be incremented or decremented. This is the
   * increment behind the generic zForEach z-loop (used for DIMENSIONS > 3; for
   * DIMENSIONS 2 and 3 zForEach3 is table-driven).
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void
  dIncrement(tarch::la::Vector<DIMENSIONS, int>& counter, int max, LoopDirection& direction);

  /**
   * Element-wise comparison for the for d-dimensional for loops
   *
   * @return true if all entries of counter are smaller max
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCompare(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * Element-wise comparison for the for d-dimensional for loops
   *
   * Element-wise comparison for the loops. Different to the other dCompare()
   * operation, this one works fine even if the max range per vector entry is
   * different.
   *
   * @return true if all entries of counter are smaller than their corresponding
   *         entries in max
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int
  dCompare(const tarch::la::Vector<DIMENSIONS, int>& counter, const tarch::la::Vector<DIMENSIONS, int>& max);

  /**
   * Exclusive element-wise comparison for d-dimensional for loops, ignoring one
   * specified dimension. This is the comparison behind the dForEachExclusive
   * loop.
   *
   * @return true if all entries of counter (ignoring the specified dimension)
   *         are smaller than max
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int
  dCompareExclusive(const tarch::la::Vector<DIMENSIONS, int>& counter, int max, int dim);

  /**
   * Map d-dimensional vector onto integer index
   *
   * @return the linearisation of the counter, i.e. the k-th component is
   *         multiplied by max^k and the results are accumulated. Component 0
   *         is therefore the fastest running index.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * Invoke body(counter) for every point of [0,max)^d, component 0 running
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
   * Visit the 2^d cube corners. body receives the corner and its scalar
   * (linearised) index.
   */
  template <typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void dForEachCorner(const Functor& body);

  /**
   * Visit the 3^d stencil in lexicographic order. body receives the point and
   * its scalar (linearised) index.
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

#include "Loop.cpph"
