// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"

namespace peano4::grid {
  class PeanoCurve;
  struct AutomatonState;
} // namespace peano4::grid

class peano4::grid::PeanoCurve {
public:
  static bool isTraversePositiveAlongAxis(const AutomatonState& state, int axis);

  static peano4::LoopDirection getLoopDirection(const AutomatonState& state);
};
