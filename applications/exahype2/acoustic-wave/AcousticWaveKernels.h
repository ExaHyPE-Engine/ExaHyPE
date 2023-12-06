// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::acousticwave {
  static double maxEigenvalue() InlineMethod { return WAVE_SPEED; }

  static void flux(const double* __restrict__ Q, int normal, double* __restrict__ F) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

    F[0] = K0 * Q[normal + 1];
    F[1] = 0.0;
    F[2] = 0.0;
#if Dimensions == 3
    F[3] = 0.0;
#endif
    F[normal + 1] = (1.0 / RHO) * Q[0];
  }

  static void sourceTerm(const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, double t, double* __restrict__ S) InlineMethod {
    S[0] = 0.0;
    S[1] = 0.0;
    S[2] = 0.0;
#if Dimensions == 3
    S[3] = 0.0;
#endif

#if Dimensions == 2
    const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({5, 5})) <= 1.0;
#else
    const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({5, 5, 5})) <= 1.0;
#endif

    if (pointSource) {
      static constexpr double t0 = 0.7;
      static constexpr double M0 = 1000.0;
      const double force = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * SIGMA * SIGMA));
      S[0]               = force;
    }
  }
} // namespace applications::exahype2::acousticwave
