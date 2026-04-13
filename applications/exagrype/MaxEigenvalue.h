// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

#include <algorithm>
#include <cmath>

namespace applications::exahype2::exagrype {
  template <class Shortcuts>
  static inline GPU_CALLABLE_METHOD double maxEigenvalue(const double* const Q, int normal);
} // namespace applications::exahype2::exagrype

#include "MaxEigenvalue.cpph"
