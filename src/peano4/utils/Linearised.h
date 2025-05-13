// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

#include <bitset>

// #define DLINEAR_USE_LOOKUP
//  Default max value for precomputed lookup tables.
//  Can be overridden at compile time: -DMaxTable=10
#ifndef MaxTable
#define MaxTable 16
#endif
static_assert(MaxTable >= 2, "MaxTable must be at least 2.");

namespace peano4 {
  namespace utils {
    constexpr int MaxIndexOfLinearisation = BasePowerExponent<MaxTable, Dimensions>::value; // Number of possible
                                                                                            // entries in a
                                                                                            // d-dimensional grid

    extern int                                lookupTableDLinearised[MaxIndexOfLinearisation * (MaxTable - 1)];
    extern tarch::la::Vector<Dimensions, int> lookupTableDDeLinearised[MaxIndexOfLinearisation * (MaxTable - 1)];

    inline GPUCallableMethod int getKeyForDLinearisedLookup(const tarch::la::Vector<Dimensions, int>& counter, int max);
    inline GPUCallableMethod int getKeyForDDeLinearisedLookup(int value, int max);

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
    inline GPUCallableMethod int dLinearised(const std::bitset<Dimensions>& counter);
    inline GPUCallableMethod int dLinearised(const tarch::la::Vector<Dimensions, int>& counter, int max);
    inline GPUCallableMethod int dLinearisedWithoutLookup(const tarch::la::Vector<Dimensions, int>& counter, int max);

    /**
     * Special 2d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    inline GPUCallableMethod int d2Linearised(const tarch::la::Vector<2, int>& counter, int max);

    /**
     * Special 3d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    inline GPUCallableMethod int d3Linearised(const tarch::la::Vector<3, int>& counter, int max);

    /**
     * Counterpart of dLinearised().
     */
    inline GPUCallableMethod tarch::la::Vector<Dimensions, int> dDelinearised(int value, int max);
    inline GPUCallableMethod tarch::la::Vector<Dimensions, int> dDelinearisedWithoutLookup(int value, int max);

    void setupLookupTableForDLinearised();
    void setupLookupTableForDDelinearised();
  } // namespace utils
} // namespace peano4

#include "Linearised.cpph"
