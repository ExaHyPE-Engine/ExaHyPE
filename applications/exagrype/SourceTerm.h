// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

#include <cmath>

namespace applications::exahype2::exagrype {
  template <class Shortcuts>
  static inline GPU_CALLABLE_METHOD void source(
    double*                                      S,
    const double* const                          Q,
    double                                       t,
    const tarch::la::Vector<DIMENSIONS, double>& x
  );
} // namespace applications::exahype2::exagrype

#include "SourceTerm.cpph"
