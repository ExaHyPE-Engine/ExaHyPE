// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "LoopDirection.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  namespace utils {
    /**
     * d-dimensional counterpart of increment operator
     *
     * This operation performs a d-dimensional increment on a given integer vector:
     * The first component of the vector is incremented. If the first component is
     * greater than max-1, the component is set zero and the next component is
     * incremented by one. This operation is used often by d-dimensional for-loops.
     */
    inline GPU_CALLABLE_METHOD void dInc(tarch::la::Vector<DIMENSIONS, int>& counter, int max);

    /**
     * d-dimensional counterpart of increment, where individual vector components have different max values
     *
     * This operation performs a d-dimensional increment on a given integer vector:
     * The first component of the vector is incremented. If the first component is
     * greater than max(0)-1, the component is set zero and the next component is
     * incremented by one. This operation is used often by d-dimensional for-loops.
     */
    inline GPU_CALLABLE_METHOD void dInc(
      tarch::la::Vector<DIMENSIONS, int>&       counter,
      const tarch::la::Vector<DIMENSIONS, int>& max
    );

    /**
     * Perform a d-dimensional increment by value increment: The first component
     * of the counter is incremented by increment. Afterwards, the operation
     * checks the first entry: If it exceeds max, its module value is set, the
     * next component is incremented by increment, and the check continues.
     */
    inline GPU_CALLABLE_METHOD void dIncByVector(tarch::la::Vector<DIMENSIONS, int>& counter, int max, int increment);

    /**
     * Same operation as dInc(tarch::la::Vector<DIMENSIONS,int>,int), but now one dimension is not taken
     * into consideration.
     */
    inline GPU_CALLABLE_METHOD void dInc(tarch::la::Vector<DIMENSIONS, int>& counter, int max, int doNotExamine);

    /**
     * Operation similar to dInc, but is given a direction bitset that identifies
     * whether the counters has to be incremented or decremented. See the dforz
     * macro for an example how to use dInc.
     */
    inline GPU_CALLABLE_METHOD void dInc(tarch::la::Vector<DIMENSIONS, int>& counter, int max, LoopDirection& direction);

    /**
     * Perform a scalar increment of a vector: The operation equals a sequence of
     * increment calls to dInc().
     */
    inline GPU_CALLABLE_METHOD void dIncByScalar(tarch::la::Vector<DIMENSIONS, int>& counter, int max, int increment);
  } // namespace utils
} // namespace peano4

#include "Increment.cpph"
