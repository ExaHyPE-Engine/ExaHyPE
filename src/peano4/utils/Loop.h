// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Compare.h"
#include "config.h"
#include "Decrement.h"
#include "Globals.h"
#include "Increment.h"
#include "Linearised.h"
#include "LoopDirection.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  namespace utils {
    /**
     * Construct start vector (0,0,....) for d-dimensional loop
     *
     * @return a vector containing zero values only.
     */
    inline GPUCallableMethod tarch::la::Vector<Dimensions, int> dStartVector();

    /**
     * @return a vector containing only zero values besides the dim-th entry. This
     *         entry is set value.
     */
    inline GPUCallableMethod tarch::la::Vector<Dimensions, int> dStartVector(int dim, int value);

    /**
     * Creates a start vector. Each component is set either 0 or max-1 depending
     * on direction: If direction is true, then the value 0 is zero.
     *
     * @return a start vector for an oscillating loop.
     */
    inline GPUCallableMethod tarch::la::Vector<Dimensions, int> dStartVector(int max, const LoopDirection& direction);
  }
}

#define dfor(counter, max) \
  for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(); peano4::utils::dCmp(counter, max); \
       peano4::utils::dInc(counter, max))

#define d2for(counter, max) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(2, max); counter##Scalar++) { \
    tarch::la::Vector<2, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 2 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= max; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

#define d3for(counter, max) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(3, max); counter##Scalar++) { \
    tarch::la::Vector<3, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 3 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= max; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

#if Dimensions == 2
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0; counter##Scalar < TwoPowerD; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter);
#else
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0, CC##counter = 0; counter##Scalar < TwoPowerD; \
       counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    counter(2)  = CC##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter); \
    CC##counter = CC##counter || (!AA##counter && !BB##counter && !CC##counter);
#endif

#define dfor3(counter) \
  for (int counter##Scalar = 0; counter##Scalar < ThreePowerD; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
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
  for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(dim, value); \
       peano4::utils::dCmp(counter, max); \
       peano4::utils::dInc(counter, max, dim))

/**
 * This is a d-dimensional z-loop. A z-loop is a d-dimensional loop the
 * counter direction changes every time an inner loop direction has changed.
 * So this is the loop corresponding to a Peano curve. The for loop is passed
 * a counter name, the number of steps to perform in each direction and a
 * direction flag that identifies the initial direction. Note that this
 * argument has to be a real variable, it might not be a constant. The
 * direction flag array identifies for each direction, whether the initial
 * loop goes along the axis or not. The type of direction is LoopDirection.
 *
 * Here are some examples for two dimensions:
 * \code
 *   LoopDirection d(3);  // equals {true,true} and identifies the standard
 *                        // Peano Leitmotiv
 *   zfor( a, 3, d ) {
 *     std::cout << a;
 *   }
 * \endcode
 * yields in [0,0],[1,0],[2,0],[2,1],[1,1],[0,1],[0,2],[1,2],[2,2].
 *
 * \code
 *   LoopDirection d(1);  // equals {true, false} and specifies a Peano curve
 *                        // from the left top to right bottom
 *   zfor( a, 3, d ) {
 *     std::cout << a;
 *   }
 * \endcode
 * yields in [0,2],[1,2],[2,2],[2,1],[1,1],[0,1],[0,0],[1,0],[2,0].
 */
#define zfor(counter, max, direction) \
  { \
    for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(max, direction); \
         peano4::utils::dCmp(counter, max); \
         peano4::utils::dInc(counter, max, direction)) {

/*
 * zfor3 is an optimised version of zfor for max = 3
 * A lookup table is used for dim=2 and dim=3, for higher dimensions
 * the standard zfor is used instead.
 */
#if Dimensions == 2
// First entry is direction, second entry is cell, third entry is tuples
static constexpr int lookupzfor3[4][9][2] = {
  {{2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}, {2, 1}, {2, 0}, {1, 0}, {0, 0}},
  {{0, 2}, {1, 2}, {2, 2}, {2, 1}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {2, 0}},
  {{2, 0}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {2, 1}, {2, 2}, {1, 2}, {0, 2}},
  {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {1, 1}, {0, 1}, {0, 2}, {1, 2}, {2, 2}}
};

#define zfor3(counter, direction) \
  { \
    tarch::la::Vector<Dimensions, int> counter; \
    int                                counter##initDir = static_cast<int>(direction.to_ulong()); \
    for (int counter##i = 0; counter##i < 9; ++counter##i) { \
      counter(0) = lookupzfor3[counter##initDir][counter##i][0]; \
      counter(1) = lookupzfor3[counter##initDir][counter##i][1];

#elif Dimensions == 3
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
    tarch::la::Vector<Dimensions, int> counter; \
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
