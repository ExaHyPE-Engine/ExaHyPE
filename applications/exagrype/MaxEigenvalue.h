// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exagrype {
  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(const State& Q, int normal);
} // namespace applications::exahype2::exagrype

#include "applications/exagrype/MaxEigenvalue.cpph"
