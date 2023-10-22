// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::euler {
  enum Variables {
    rho = 0,
    u,
    v,
#if Dimensions == 3
    w,
#endif
    e
  };

  static inline double maxEigenvalue(const double* __restrict__ Q, int normal) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[rho] != Q[rho]);
    assertion(Q[rho] > 0);
#endif

    const double irho = 1.0 / std::abs(Q[rho]);
    const double p = (GAMMA - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v]
#if Dimensions == 3
      + Q[w] * Q[w]
#endif
    ));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif

    const double c      = std::sqrt(GAMMA * std::abs(p) * irho);
    const double u_n    = Q[normal + 1] * irho;
    const double result = std::max(std::abs(u_n - c), std::abs(u_n + c));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(result, 0.0));
#endif

    return result;
  }

  static inline void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, x, t, normal);
    assertion3(normal < Dimensions, x, t, normal);
    assertion(Q[rho] != Q[rho]);
    assertion(Q[rho] > 0);
#endif

    const double irho = 1.0 / Q[rho];
    const double p = (GAMMA - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v]
#if Dimensions == 3
      + Q[w] * Q[w]
#endif
    ));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif

    const double coeff = irho * Q[normal + 1];
    F[rho]             = coeff * Q[rho];
    F[u]               = coeff * Q[u];
    F[v]               = coeff * Q[v];
#if Dimensions == 3
    F[w] = coeff * Q[w];
#endif
    F[e] = coeff * Q[e] + coeff * p;
    F[normal + 1] += p;
  }
}; // namespace applications::exahype2::euler
