// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Globals.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <bitset>

// #define DLINEAR_USE_LOOKUP
#ifndef MAX_TABLE
#define MAX_TABLE 16
#endif
static_assert(MAX_TABLE >= 2, "MAX_TABLE must be at least 2.");

namespace peano4 {
  namespace utils {
    constexpr int MaxIndexOfLinearisation = BasePowerExponent<MAX_TABLE, DIMENSIONS>::value; // Number of possible
                                                                                             // entries in a
                                                                                             // d-dimensional grid

    extern int                                lookupTableDLinearised[MaxIndexOfLinearisation * (MAX_TABLE - 1)];
    extern tarch::la::Vector<DIMENSIONS, int> lookupTableDDeLinearised[MaxIndexOfLinearisation * (MAX_TABLE - 1)];

    INLINE_METHOD GPU_CALLABLE_METHOD int getKeyForDLinearisedLookup(
      const tarch::la::Vector<DIMENSIONS, int>& counter,
      int                                       max
    );
    INLINE_METHOD GPU_CALLABLE_METHOD int getKeyForDDeLinearisedLookup(int value, int max);

    /**
     * Map d-dimensional vector onto integer index
     *
     * This operation is called pretty often and, thus, might cause a significant
     * slowdown in the overall performance. Therefore, I introduced a aggressive
     * optimisation based on lookup tables. This optimisation is switched on if
     * DLINEAR_USE_LOOKUP is defined.
     *
     * @return the linearisation of the counter, i.e. the k-th component is
     *         multiplied by max^k and the results are accumulated.
     */
    INLINE_METHOD int                     dLinearised(const std::bitset<DIMENSIONS>& counter);
    INLINE_METHOD GPU_CALLABLE_METHOD int dLinearised(const tarch::la::Vector<DIMENSIONS, int>& counter, int max);
    INLINE_METHOD GPU_CALLABLE_METHOD int dLinearisedWithoutLookup(
      const tarch::la::Vector<DIMENSIONS, int>& counter,
      int                                       max
    );

    /**
     * Special 2d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    INLINE_METHOD GPU_CALLABLE_METHOD int d2Linearised(const tarch::la::Vector<2, int>& counter, int max);

    /**
     * Special 3d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    INLINE_METHOD GPU_CALLABLE_METHOD int d3Linearised(const tarch::la::Vector<3, int>& counter, int max);

    /**
     * Counterpart of dLinearised().
     */
    INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dDelinearised(int value, int max);
    INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> dDelinearisedWithoutLookup(int value, int max);

    void setupLookupTableForDLinearised();
    void setupLookupTableForDDelinearised();
  } // namespace utils
} // namespace peano4

#include "Linearised.cpph"
