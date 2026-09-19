// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exagrype {
  template <class Shortcuts>
  INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const double* NOALIAS Q,
    const double* NOALIAS deltaQ,
    int                   normal,
    double* NOALIAS       BTimesDeltaQ
  );
} // namespace applications::exahype2::exagrype

#include "applications/exagrype/NonconservativeProduct.cpph"
