// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

#include <cmath>

namespace applications::exahype2::exagrype {
  template <class Shortcuts, int NumberOfUnknowns>
  static inline GPU_CALLABLE_METHOD void ncp(
    double*             BgradQ,
    const double* const Q,
    const double* const deltaQ,
    int                 normal
  );
} // namespace applications::exahype2::exagrype

#include "NonconservativeProduct.cpph"
