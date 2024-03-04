// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::elasticwave {
  static inline double maxEigenvalue() InlineMethod { return std::max(std::abs(P_WAVE_SPEED), std::abs(S_WAVE_SPEED)); }

  static inline void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    static constexpr double Mu     = RHO * S_WAVE_SPEED * S_WAVE_SPEED;
    static constexpr double Lambda = RHO * P_WAVE_SPEED * P_WAVE_SPEED - 2.0 * Mu;

    const double u   = Q[0];
    const double v   = Q[1];
    const double sxx = Q[2];
    const double syy = Q[3];
    const double sxy = Q[4];

    switch (normal) {
    case 0:
      F[0] = -1.0 / RHO * sxx;
      F[1] = -1.0 / RHO * sxy;
      F[2] = -(2.0 * Mu + Lambda) * u;
      F[3] = -Lambda * u;
      F[4] = -Mu * v;
      break;
    case 1:
      F[0] = -1.0 / RHO * sxy;
      F[1] = -1.0 / RHO * syy;
      F[2] = -Lambda * v;
      F[3] = -(2.0 * Mu + Lambda) * v;
      F[4] = -Mu * u;
      break;
    }
  }

  static void sourceTerm(const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, double t, double* __restrict__ S) InlineMethod {
    S[0] = 0.0;
    S[1] = 0.0;
    S[2] = 0.0;
    S[3] = 0.0;
    S[4] = 0.0;

    const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({10, 10})) <= 1.0;
    if (pointSource) {
      static constexpr double t0 = 0.1;
      static constexpr double M0 = 1000.0;
      const double force = M0 * t / (t0 * t0) * std::exp(-t / t0);
      S[4]               = force;
    }
  }
} // namespace applications::exahype2::elasticwave
