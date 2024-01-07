// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::acoustics {
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double maxEigenvalue() InlineMethod { return WAVE_SPEED; }

  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

    F[VARS_p] = K0 * Q[normal + 1];
    F[VARS_u] = 0.0;
    F[VARS_v] = 0.0;
#if Dimensions == 3
    F[VARS_w] = 0.0;
#endif
    F[normal + 1] = (1.0 / RHO) * Q[0];
  }

  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void sourceTerm(
    const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, double t, double* __restrict__ S
  ) InlineMethod {
    S[VARS_p] = 0.0;
    S[VARS_u] = 0.0;
    S[VARS_v] = 0.0;
#if Dimensions == 3
    S[VARS_w] = 0.0;
#endif

    if (tarch::la::norm2(x - SOURCE_LOCATION) < SOURCE_RADIUS) {
      S[VARS_p] = SOURCE_M0 * std::exp(-((t - SOURCE_t0) * (t - SOURCE_t0)) / (2.0 * SIGMA * SIGMA));
    }
  }
} // namespace applications::exahype2::acoustics
