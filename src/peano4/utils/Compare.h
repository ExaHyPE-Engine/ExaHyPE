// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  namespace utils {
    /**
     * Element-wise comparison for the for d-dimensional for loops
     *
     * @return true if all entries of counter are smaller max
     */
    inline GPUCallableMethod int dCmp(const tarch::la::Vector<Dimensions, int>& counter, int max);

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
    inline GPUCallableMethod int dCmp(
      const tarch::la::Vector<Dimensions, int>& counter,
      const tarch::la::Vector<Dimensions, int>& max
    );

    /**
     * Compares two vectors with regards to their linearised value
     *
     * @returns true, if dLinearised(counter, XXX) < dLinearised(max, XXX)
     */
    inline GPUCallableMethod bool dCmpLinearOrder(
      const tarch::la::Vector<Dimensions, int>& counter,
      const tarch::la::Vector<Dimensions, int>& max
    );
  } // namespace utils
} // namespace peano4

#include "Compare.cpph"
