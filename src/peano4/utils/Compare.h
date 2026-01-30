// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  namespace utils {
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
     * Compares two vectors with regards to their linearised value
     *
     * @returns true, if dLinearised(counter, XXX) < dLinearised(max, XXX)
     */
    INLINE_METHOD GPU_CALLABLE_METHOD bool dCmpLinearOrder(
      const tarch::la::Vector<DIMENSIONS, int>& counter,
      const tarch::la::Vector<DIMENSIONS, int>& max
    );
  } // namespace utils
} // namespace peano4

#include "Compare.cpph"
