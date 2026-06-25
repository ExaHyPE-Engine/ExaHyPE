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
   * Is used by the z-loop. See the macros zfor and zfor3.
   */
  typedef std::bitset<DIMENSIONS> LoopDirection;

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
   * the generic zfor z-loop.
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
  INLINE_METHOD GPU_CALLABLE_METHOD void dInc(tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * d-dimensional counterpart of increment, where individual vector components have different max values
   *
   * This operation performs a d-dimensional increment on a given integer vector:
   * The first component of the vector is incremented. If the first component is
   * greater than max(0)-1, the component is set zero and the next component is
   * incremented by one. This operation is used often by d-dimensional for-loops.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dInc(
    tarch::la::Vector<DIMENSIONS, int>&       counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  /**
   * Same operation as dInc(tarch::la::Vector<DIMENSIONS,int>,int), but now one dimension is not taken
   * into consideration. This is the increment behind the dfore (exclusive) loop.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dInc(tarch::la::Vector<DIMENSIONS, int>& counter, int max, int doNotExamine);

  /**
   * Operation similar to dInc, but is given a direction bitset that identifies
   * whether the counters has to be incremented or decremented. This is the
   * increment behind the generic zfor z-loop (used for DIMENSIONS > 3; for
   * DIMENSIONS 2 and 3 zfor3 is table-driven). See the zfor macro for an
   * example how to use it.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void dInc(
    tarch::la::Vector<DIMENSIONS, int>& counter,
    int                                 max,
    LoopDirection&                      direction
  );

  /**
   * Element-wise comparison for the for d-dimensional for loops
   *
   * @return true if all entries of counter are smaller max
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCmp(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);

  /**
   * Element-wise comparison for the for d-dimensional for loops
   *
   * Element-wise comparison for the loops. Different to the other dCmp()
   * operation, this one works fine even if the max range per vector entry is
   * different.
   *
   * @return true if all entries of counter are smaller than their corresponding
   *         entries in max
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dCmp(
    const tarch::la::Vector<DIMENSIONS, int>& counter,
    const tarch::la::Vector<DIMENSIONS, int>& max
  );

  /**
   * Map d-dimensional vector onto integer index
   *
   * @return the linearisation of the counter, i.e. the k-th component is
   *         multiplied by max^k and the results are accumulated. Component 0
   *         is therefore the fastest running index.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);
} // namespace peano4

#define dfor(counter, max) \
  for (tarch::la::Vector<DIMENSIONS, int> counter = ::peano4::dStartVector(); \
       ::peano4::dCmp(counter, max); \
       ::peano4::dInc(counter, max))

#if DIMENSIONS == 2
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0; counter##Scalar < TWO_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<DIMENSIONS, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter);
#else
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0, CC##counter = 0; counter##Scalar < TWO_POWER_D; \
       counter##Scalar++) { \
    tarch::la::Vector<DIMENSIONS, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    counter(2)  = CC##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter); \
    CC##counter = CC##counter || (!AA##counter && !BB##counter && !CC##counter);
#endif

#define dfor3(counter) \
  for (int counter##Scalar = 0; counter##Scalar < THREE_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<DIMENSIONS, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = DIMENSIONS - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 3; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

/**
 * I prefer to use this macro for dforx instead of a closing bracket as many
 * syntax parser fail otherwise.
 */
#define enddforx }

/**
 * This is an exclusive d-dimensional for loop. Exclusive means, there is one
 * dimension that is not manipulated during the for loop. This dimension
 * (entry of the counter) is specified by dim and has the value value
 * throughout the for-loop.
 */
#define dfore(counter, max, dim, value) \
  for (tarch::la::Vector<DIMENSIONS, int> counter = peano4::dStartVector(dim, value); \
       peano4::dCmp(counter, max); \
       peano4::dInc(counter, max, dim))

/**
 * Generic d-dimensional z-loop.
 *
 * A z-loop is a d-dimensional loop whose counter direction changes every time an
 * inner loop direction has changed, i.e. the loop corresponding to a Peano
 * curve. The loop is passed a counter name, the number of steps to perform in
 * each direction (max) and a direction flag that identifies the initial
 * direction. Note that direction has to be a real variable, not a constant. The
 * direction flag (type LoopDirection) identifies for each axis whether the
 * initial loop runs along the axis.
 *
 * This generic variant works for any number of dimensions and any max. For the
 * common case max = 3 and DIMENSIONS 2 or 3 use the table-driven zfor3 instead,
 * which is what zfor3 falls back to here for DIMENSIONS > 3.
 *
 * \code
 *   LoopDirection d(3);  // equals {true,true}, the standard Peano Leitmotiv
 *   zfor( a, 3, d ) {
 *     std::cout << a;
 *   } endzfor
 * \endcode
 * yields in [0,0],[1,0],[2,0],[2,1],[1,1],[0,1],[0,2],[1,2],[2,2].
 */
#define zfor(counter, max, direction) \
  { \
    for (tarch::la::Vector<DIMENSIONS, int> counter = peano4::dStartVector(max, direction); \
         peano4::dCmp(counter, max); \
         peano4::dInc(counter, max, direction)) {

/**
 * d-dimensional z-loop (Peano space-filling-curve order) for max = 3.
 *
 * A z-loop is a d-dimensional loop whose counter direction changes every time an
 * inner loop direction has changed, i.e. the loop corresponding to a Peano
 * curve. It is passed a counter name and a direction flag of type LoopDirection
 * that identifies, per axis, whether the initial loop runs along the axis. For
 * DIMENSIONS 2 and 3 it is fully table-driven (lookupzfor3); for higher
 * dimensions it falls back to the generic zfor.
 *
 * Here are some examples for two dimensions:
 * \code
 *   LoopDirection d(3);  // equals {true,true} and identifies the standard
 *                        // Peano Leitmotiv
 *   zfor3( a, d ) {
 *     std::cout << a;
 *   } endzfor
 * \endcode
 * yields in [0,0],[1,0],[2,0],[2,1],[1,1],[0,1],[0,2],[1,2],[2,2].
 *
 * \code
 *   LoopDirection d(1);  // equals {true, false} and specifies a Peano curve
 *                        // from the left top to right bottom
 *   zfor3( a, d ) {
 *     std::cout << a;
 *   } endzfor
 * \endcode
 * yields in [0,2],[1,2],[2,2],[2,1],[1,1],[0,1],[0,0],[1,0],[2,0].
 */
#if DIMENSIONS == 2
// First entry is direction, second entry is cell, third entry is tuples
static constexpr int lookupzfor3[4][9][2] = {
  {{2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}, {2, 1}, {2, 0}, {1, 0}, {0, 0}},
  {{0, 2}, {1, 2}, {2, 2}, {2, 1}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {2, 0}},
  {{2, 0}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {2, 1}, {2, 2}, {1, 2}, {0, 2}},
  {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {1, 1}, {0, 1}, {0, 2}, {1, 2}, {2, 2}}
};

#define zfor3(counter, direction) \
  { \
    tarch::la::Vector<DIMENSIONS, int> counter; \
    int                                counter##initDir = static_cast<int>(direction.to_ulong()); \
    for (int counter##i = 0; counter##i < 9; ++counter##i) { \
      counter(0) = lookupzfor3[counter##initDir][counter##i][0]; \
      counter(1) = lookupzfor3[counter##initDir][counter##i][1];

#elif DIMENSIONS == 3
static constexpr int lookupzfor3[8][27][3] = {
  {{2, 2, 2}, {1, 2, 2}, {0, 2, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 0, 2}, {1, 0, 2}, {0, 0, 2},
   {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 2, 1}, {1, 2, 1}, {2, 2, 1},
   {2, 2, 0}, {1, 2, 0}, {0, 2, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 0, 0}, {1, 0, 0}, {0, 0, 0}},
  {{0, 2, 2}, {1, 2, 2}, {2, 2, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2},
   {2, 0, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 2, 1}, {1, 2, 1}, {0, 2, 1},
   {0, 2, 0}, {1, 2, 0}, {2, 2, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}, {1, 0, 0}, {2, 0, 0}},
  {{2, 0, 2}, {1, 0, 2}, {0, 0, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 2, 2}, {1, 2, 2}, {0, 2, 2},
   {0, 2, 1}, {1, 2, 1}, {2, 2, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1},
   {2, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 2, 0}, {1, 2, 0}, {0, 2, 0}},
  {{0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 2, 2}, {1, 2, 2}, {2, 2, 2},
   {2, 2, 1}, {1, 2, 1}, {0, 2, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 0, 1}, {1, 0, 1}, {0, 0, 1},
   {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 2, 0}, {1, 2, 0}, {2, 2, 0}},
  {{2, 2, 0}, {1, 2, 0}, {0, 2, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 0, 0}, {1, 0, 0}, {0, 0, 0},
   {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 2, 1}, {1, 2, 1}, {2, 2, 1},
   {2, 2, 2}, {1, 2, 2}, {0, 2, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 0, 2}, {1, 0, 2}, {0, 0, 2}},
  {{0, 2, 0}, {1, 2, 0}, {2, 2, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}, {1, 0, 0}, {2, 0, 0},
   {2, 0, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 2, 1}, {1, 2, 1}, {0, 2, 1},
   {0, 2, 2}, {1, 2, 2}, {2, 2, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}},
  {{2, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 2, 0}, {1, 2, 0}, {0, 2, 0},
   {0, 2, 1}, {1, 2, 1}, {2, 2, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1},
   {2, 0, 2}, {1, 0, 2}, {0, 0, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 2, 2}, {1, 2, 2}, {0, 2, 2}},
  {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 2, 0}, {1, 2, 0}, {2, 2, 0},
   {2, 2, 1}, {1, 2, 1}, {0, 2, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 0, 1}, {1, 0, 1}, {0, 0, 1},
   {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 2, 2}, {1, 2, 2}, {2, 2, 2}}
};

#define zfor3(counter, direction) \
  { \
    tarch::la::Vector<DIMENSIONS, int> counter; \
    int                                counter##initDir = static_cast<int>(direction.to_ulong()); \
    for (int counter##i = 0; counter##i < 27; ++counter##i) { \
      counter(0) = lookupzfor3[counter##initDir][counter##i][0]; \
      counter(1) = lookupzfor3[counter##initDir][counter##i][1]; \
      counter(2) = lookupzfor3[counter##initDir][counter##i][2];

#else
#define zfor3(counter, direction) zfor(counter, 3, direction)
#endif

#define endzfor \
  } \
  }

#include "Loop.cpph"
