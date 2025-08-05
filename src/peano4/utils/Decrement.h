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
     * d-dimensional counterpart of decrement operator
     *
     * This operation performs a d-dimensional decrement on a given integer vector:
     * The first component of the vector is decremented. If the first component is
     * smaller than 0, the component is set to max and the next component is
     * decremented by one.
     */
    inline GPU_CALLABLE_METHOD void dDec(tarch::la::Vector<DIMENSIONS, int>& counter, int max);
  } // namespace utils
} // namespace peano4

#include "Decrement.cpph"
