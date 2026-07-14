// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exagrype {
  /**
   * A postprocessing routine which pushes the solution back into the
   * area of the CCZ4 constraints. This is a local operation that we can
   * invoke after each time step per cell.
   *
   * It works on the full set of unknowns of the first-order formulation
   */
  static inline GPU_CALLABLE_METHOD void enforceCCZ4constraints(double* __restrict__ newQ);
} // namespace applications::exahype2::exagrype

#include "EnforceCCZ4Constraints.cpph"
