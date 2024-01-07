// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "VariableShortcuts.h"

namespace applications::exahype2::euler {
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double maxEigenvalue(const double* __restrict__ Q, int normal) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[VARS_rho] != Q[VARS_rho]);
    assertion(Q[VARS_rho] > 0);
#endif

    const double irho = 1.0 / std::abs(Q[VARS_rho]);
    const double p = (GAMMA - 1) * (Q[VARS_e] - 0.5 * irho * (Q[VARS_u] * Q[VARS_u] + Q[VARS_v] * Q[VARS_v]
#if Dimensions == 3
      + Q[VARS_w] * Q[VARS_w]
#endif
    ));

#ifdef GPUOffloadingOff
    assertion(::tarch::la::greaterEquals(p, 0.0));
#endif

    const double c      = std::sqrt(GAMMA * std::abs(p) * irho);
    const double u_n    = Q[normal + 1] * irho;
    const double result = std::max(std::abs(u_n - c), std::abs(u_n + c));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(result, 0.0));
#endif

    return result;
  }

  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[VARS_rho] != Q[VARS_rho]);
    assertion(Q[VARS_rho] > 0);
#endif

    const double irho = 1.0 / Q[VARS_rho];
    const double p = (GAMMA - 1) * (Q[VARS_e] - 0.5 * irho * (Q[VARS_u] * Q[VARS_u] + Q[VARS_v] * Q[VARS_v]
#if Dimensions == 3
      + Q[VARS_w] * Q[VARS_w]
#endif
    ));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif

    const double coeff = irho * Q[normal + 1];

    F[VARS_rho] = coeff * Q[VARS_rho];
    F[VARS_u] = coeff * Q[VARS_u];
    F[VARS_v] = coeff * Q[VARS_v];
#if Dimensions == 3
    F[VARS_w] = coeff * Q[VARS_w];
    F[VARS_e] = coeff * Q[VARS_e] + coeff * p;
#endif
    F[VARS_e] = coeff * Q[VARS_e] + coeff * p;

    F[normal + 1] += p;
  }
} // namespace applications::exahype2::euler
