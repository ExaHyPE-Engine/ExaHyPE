// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeanoCurve.h"

#include "peano4/grid/AutomatonState.h"
#include "tarch/Assertions.h"

bool peano4::grid::PeanoCurve::isTraversePositiveAlongAxis(const peano4::grid::AutomatonState& state, int axis) {
  assertion(axis >= 0);
  assertion(axis < DIMENSIONS);
  bool evenParity = true;
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d == axis) {
      continue;
    }
    if (state.getEvenFlag(d)) {
      evenParity = not evenParity;
    }
  }
  return evenParity;
}

/**
 * @return one set bit per axis along which the traversal runs in positive
 *         direction.
 */
peano4::LoopDirection peano4::grid::PeanoCurve::getLoopDirection(const peano4::grid::AutomatonState& state) {
  peano4::LoopDirection result;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    result[axis] = isTraversePositiveAlongAxis(state, axis);
  }
  return result;
}
