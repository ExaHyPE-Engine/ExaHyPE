// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "VariableShortcuts.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::euler {
  static inline double maxEigenvalue(const double* __restrict__ Q, int normal) InlineMethod {
    using s = VariableShortcuts;

#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[s::rho] != Q[s::rho]);
    assertion(Q[s::rho] > 0);
#endif

    const double irho = 1.0 / std::abs(Q[s::rho]);
#if Dimensions == 3
    const double p
      = (gamma - 1)
        * (Q[s::e] - 0.5 * irho * (Q[s::v] * Q[s::v] + Q[s::v + 1] * Q[s::v + 1] + Q[s::v + 2] * Q[s::v + 2]));
#else
    const double p = (gamma - 1) * (Q[s::e] - 0.5 * irho * (Q[s::v] * Q[s::v] + Q[s::v + 1] * Q[s::v + 1]));
#endif

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif
    const double c = std::sqrt(gamma * std::abs(p) * irho);

    const double u_n    = Q[normal + 1] * irho;
    const double result = std::max(std::abs(u_n - c), std::abs(u_n + c));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(result, 0.0));
#endif

    return result;
  }

  static inline void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
    using s = VariableShortcuts;

#ifdef GPUOffloadingOff
    assertion3(normal >= 0, x, t, normal);
    assertion3(normal < Dimensions, x, t, normal);
    assertion(Q[s::rho] != Q[s::rho]);
    assertion(Q[s::rho] > 0);
#endif

    const double irho = 1.0 / Q[s::rho];
#if Dimensions == 3
    const double p
      = (gamma - 1)
        * (Q[s::e] - 0.5 * irho * (Q[s::v] * Q[s::v] + Q[s::v + 1] * Q[s::v + 1] + Q[s::v + 2] * Q[s::v + 2]));
#else
    const double p = (gamma - 1) * (Q[s::e] - 0.5 * irho * (Q[s::v] * Q[s::v] + Q[s::v + 1] * Q[s::v + 1]));
#endif

    const double coeff = irho * Q[normal + 1];
    F[s::rho]          = coeff * Q[s::rho];
    F[s::v]            = coeff * Q[s::v];
    F[s::v + 1]        = coeff * Q[s::v + 1];

#if Dimensions == 3
    F[s::v + 2] = coeff * Q[s::v + 2];
    F[s::e]     = coeff * Q[s::e] + coeff * p;
#else
    F[s::e] = coeff * Q[s::e] + coeff * p;
#endif

    F[normal + 1] += p;
  }
}; // namespace applications::exahype2::euler
