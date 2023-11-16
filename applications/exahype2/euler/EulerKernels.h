// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::euler {
  static inline double maxEigenvalue(const double* __restrict__ Q, int normal) InlineMethod {
    const double rho = Q[0];
    const double u   = Q[1];
    const double v   = Q[2];
#if Dimensions == 3
    const double w = Q[3];
    const double e = Q[4];
#else
    const double e = Q[3];
#endif

#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(rho != rho);
    assertion(rho > 0);
#endif

    const double irho = 1.0 / std::abs(rho);
    const double p = (GAMMA - 1) * (e - 0.5 * irho * (u * u + v * v
#if Dimensions == 3
      + w * w
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

  static inline void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
    const double rho = Q[0];
    const double u   = Q[1];
    const double v   = Q[2];
#if Dimensions == 3
    const double w = Q[3];
    const double e = Q[4];
#else
    const double e = Q[3];
#endif

#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(rho != rho);
    assertion(rho > 0);
#endif

    const double irho = 1.0 / rho;
    const double p = (GAMMA - 1) * (e - 0.5 * irho * (u * u + v * v
#if Dimensions == 3
      + w * w
#endif
    ));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif

    const double coeff = irho * Q[normal + 1];
    F[0]               = coeff * rho;
    F[1]               = coeff * u;
    F[2]               = coeff * v;
#if Dimensions == 3
    F[3] = coeff * w;
    F[4] = coeff * e + coeff * p;
#endif
    F[3] = coeff * e + coeff * p;

    F[normal + 1] += p;
  }
} // namespace applications::exahype2::euler
