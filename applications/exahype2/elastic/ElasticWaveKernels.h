// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "VariableShortcuts.h"

namespace applications::exahype2::elastic {
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double maxEigenvalue(
    const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal
  ) InlineMethod {
    return std::max(std::abs(P_WAVE_SPEED), std::abs(S_WAVE_SPEED));
  }


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    static constexpr double Mu     = RHO * S_WAVE_SPEED * S_WAVE_SPEED;
    static constexpr double Lambda = RHO * P_WAVE_SPEED * P_WAVE_SPEED - 2.0 * Mu;

    switch (normal) {
    case 0:
      F[VARS_vx]  = -1.0 / RHO * Q[VARS_sxx];
      F[VARS_vy]  = -1.0 / RHO * Q[VARS_sxy];
      F[VARS_sxx] = -(2.0 * Mu + Lambda) * Q[VARS_vx];
      F[VARS_syy] = -Lambda * Q[VARS_vx];
      F[VARS_sxy] = -Mu * Q[VARS_vy];
      break;
    case 1:
      F[VARS_vx]  = -1.0 / RHO * Q[VARS_sxy];
      F[VARS_vy]  = -1.0 / RHO * Q[VARS_syy];
      F[VARS_sxx] = -Lambda * Q[VARS_vy];
      F[VARS_syy] = -(2.0 * Mu + Lambda) * Q[VARS_vy];
      F[VARS_sxy] = -Mu * Q[VARS_vx];
      break;
    }
  }


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void sourceTerm(
    const double* const __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S
  ) InlineMethod {
    S[VARS_vx]  = 0.0;
    S[VARS_vy]  = 0.0;
    S[VARS_sxx] = 0.0;
    S[VARS_syy] = 0.0;
    S[VARS_sxy] = 0.0;

    if (tarch::la::norm2(x - SOURCE_LOCATION) < SOURCE_RADIUS) {
      S[VARS_sxy] = SOURCE_M0 * t / (SOURCE_t0 * SOURCE_t0) * std::exp(-t / SOURCE_t0);
    }
  }
} // namespace applications::exahype2::elastic
