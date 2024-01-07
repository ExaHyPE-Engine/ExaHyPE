// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::advection {
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double maxEigenvalue() InlineMethod { return ADVECTION_SPEED; }

  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    F[VARS_u] = 0.0;
    F[VARS_v] = 0.0;
#if Dimensions == 3
    F[VARS_w] = 0.0;
#endif

    F[normal] = Q[normal];
  }
} // namespace applications::exahype2::advection
