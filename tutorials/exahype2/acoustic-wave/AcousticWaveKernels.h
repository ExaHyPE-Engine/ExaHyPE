// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace exahype2::acousticwave {
  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       c
  ) InlineMethod {
    return c;
  }

  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       c
  ) InlineMethod {
    return maxEigenvalue(Q, faceCentre, t, dt, normal, c);
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       rho,
    double                                       c,
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, faceCentre, t, normal);
    assertion3(normal < Dimensions, faceCentre, t, normal);
#endif

    const double K_0 = rho * c * c;

    F[0]          = K_0 * Q[normal + 1];
    F[1]          = 0.0;
    F[2]          = 0.0;
#if Dimensions == 3
    F[3]          = 0.0;
#endif
    F[normal + 1] = (1 / rho) * Q[0];
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       rho,
    double                                       c,
    double* __restrict__ F
  ) InlineMethod {
    flux(Q, faceCentre, t, dt, normal, rho, c, F);
  }
} // namespace exahype2::acousticwave
