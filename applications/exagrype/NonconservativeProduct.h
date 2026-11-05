// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exagrype {
  template <typename State, typename Difference, typename NonconservativeProduct>
  INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const State&            Q,
    const Difference&       deltaQ,
    int                     normal,
    NonconservativeProduct& BTimesDeltaQ
  );
} // namespace applications::exahype2::exagrype

#include "applications/exagrype/NonconservativeProduct.cpph"
